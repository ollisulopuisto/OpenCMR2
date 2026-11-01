package io.github.pablocpas.opencmr2

import android.app.Activity
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Rect
import android.view.View
import android.view.ViewGroup
import android.widget.FrameLayout
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.Robolectric
import org.robolectric.RobolectricTestRunner
import org.robolectric.Shadows
import org.robolectric.annotation.Config
import org.robolectric.annotation.GraphicsMode
import java.io.File
import java.util.zip.Deflater

/**
 * LOCAL-ONLY render harness (not committed): lays out the real TouchControls
 * overlay at real screen geometries under Robolectric, checks the laid-out
 * pad frames and hit-area model, and writes software-rendered PNGs to
 * /tmp/render for visual inspection. Bitmap.compress is shadowed as a no-op
 * under Robolectric, so PNGs are encoded by hand from getPixels().
 */
@RunWith(RobolectricTestRunner::class)
@Config(sdk = [34])
@GraphicsMode(GraphicsMode.Mode.NATIVE)
class TouchControlsRenderTest {

    private data class Variant(
        val name: String,
        val scale: Float = 1f,
        val marginX: Float = 0f,
        val marginY: Float = 0f,
        val opacity: Float = 1f,
        val avoidCutout: Boolean = true,
        val cutout: Rect? = null,
        val width: Int = 2400,
        val height: Int = 1080,
        val density: Float = 2.625f,  // ~420dpi, a typical 1080x2400 phone
    )

    private fun run(v: Variant) {
        val activity = Robolectric.buildActivity(Activity::class.java).setup().get()
        activity.resources.displayMetrics.density = v.density
        activity.resources.displayMetrics.scaledDensity = v.density
        activity.resources.displayMetrics.densityDpi = (v.density * 160).toInt()
        ControllerProfiles.touchScale(activity, v.scale)
        ControllerProfiles.touchMarginX(activity, v.marginX)
        ControllerProfiles.touchMarginY(activity, v.marginY)
        ControllerProfiles.touchOpacity(activity, v.opacity)
        ControllerProfiles.touchAvoidCutout(activity, v.avoidCutout)

        val parent = FrameLayout(activity)
        val controls = TouchControls(activity, parent)

        // Inject the display cutout the insets listener would deliver.
        if (v.cutout != null) {
            val f = TouchControls::class.java.getDeclaredField("cutout")
            f.isAccessible = true
            (f.get(controls) as Rect).set(v.cutout)
        }
        val apply = TouchControls::class.java.getDeclaredMethod("applySettings")
        apply.isAccessible = true

        val wms = View.MeasureSpec.makeMeasureSpec(v.width, View.MeasureSpec.EXACTLY)
        val hms = View.MeasureSpec.makeMeasureSpec(v.height, View.MeasureSpec.EXACTLY)
        val looper = Shadows.shadowOf(android.os.Looper.getMainLooper())
        parent.measure(wms, hms)
        parent.layout(0, 0, v.width, v.height)
        looper.idle()
        if (v.cutout != null) apply.invoke(controls)
        parent.measure(wms, hms)
        parent.layout(0, 0, v.width, v.height)
        looper.idle()

        val density = activity.resources.displayMetrics.density
        val padsField = TouchControls::class.java.getDeclaredField("pads")
        padsField.isAccessible = true
        @Suppress("UNCHECKED_CAST")
        val pads = padsField.get(controls) as List<Pair<View, Boolean>>
        assertEquals(2, pads.size)

        val frames = pads.map { (pad, isLeft) ->
            val lp = pad.layoutParams as FrameLayout.LayoutParams
            val left = if (isLeft) lp.leftMargin else v.width - lp.rightMargin - lp.width
            isLeft to Rect(left, v.height - lp.bottomMargin - lp.height,
                left + lp.width, v.height - lp.bottomMargin)
        }
        val (lf, rf) = frames[0].second to frames[1].second
        assertTrue(frames[0].first && frames[0].second.left >= 0)
        assertTrue("left pad on screen: $lf", lf.left >= 0 && lf.right <= v.width && lf.bottom <= v.height)
        assertTrue("right pad on screen: $rf", rf.left >= 0 && rf.right <= v.width && rf.bottom <= v.height)
        assertTrue("pads do not overlap: $lf vs $rf", lf.right <= rf.left)

        // Hit-area model from RadialGamePad 2.0.0 source: primary dial circle
        // radius = (view - 16dp) / 2.652; 4 buttons partition the circle into
        // nearest-anchor wedges; wedge chord ~ radius * sqrt(2) at 90 degrees.
        val padPx = pads[1].first.layoutParams.width
        val padDp = padPx / density
        val radiusDp = (padDp - 16f) / 2.652f
        val wedgeDp = radiusDp * Math.sqrt(2.0).toFloat()
        val scale = ControllerProfiles.touchScale(activity)
        if (scale <= 0.81f) {
            assertTrue("wedge ${wedgeDp}dp >= 48dp at min scale on ${v.name}", wedgeDp >= 48f)
        }

        // Render the overlay to real pixels.
        val overlayField = TouchControls::class.java.getDeclaredField("overlay")
        overlayField.isAccessible = true
        val overlay = overlayField.get(controls) as FrameLayout
        System.out.println("DIAG overlay w=${overlay.width} h=${overlay.height} vis=${overlay.visibility} alpha=${overlay.alpha} children=${overlay.childCount} parentW=${parent.width}")
        for ((pad, isLeft) in pads) System.out.println("DIAG pad isLeft=$isLeft w=${pad.width} h=${pad.height} vis=${pad.visibility} left=${pad.left} top=${pad.top} lp=${pad.layoutParams.width}x${pad.layoutParams.height}")
        val bmp = Bitmap.createBitmap(v.width, v.height, Bitmap.Config.ARGB_8888)
        val canvas = Canvas(bmp)
        // Detached views skip layout traversal, so measure/lay out each pad
        // explicitly at its production frame and composite it ourselves.
        for ((i, entry) in pads.withIndex()) {
            val (pad, _) = entry
            val f = frames[i].second
            pad.forceLayout()
            pad.measure(View.MeasureSpec.makeMeasureSpec(f.width(), View.MeasureSpec.EXACTLY),
                View.MeasureSpec.makeMeasureSpec(f.height(), View.MeasureSpec.EXACTLY))
            pad.layout(0, 0, f.width(), f.height())
            System.out.println("DIAG2 pad$i ${f.width()}x${f.height()} at $f")
            canvas.save()
            canvas.translate(f.left.toFloat(), f.top.toFloat())
            pad.draw(canvas)
            canvas.restore()
        }
        val px = IntArray(v.width * v.height)
        bmp.getPixels(px, 0, v.width, 0, 0, v.width, v.height)
        var lit = 0
        for (p in px) if ((p ushr 24) > 8) lit++
        assertTrue("overlay drew visible pixels on ${v.name} (lit=$lit)", lit > 10000)
        writePng(File("/tmp/render/${v.name}.png"), px, v.width, v.height)
        File("/tmp/render/${v.name}.txt").writeText(
            "density=$density padPx=$padPx padDp=${"%.1f".format(padDp)} wedgeDp=${"%.1f".format(wedgeDp)}\n" +
            "left=$lf right=$rf opacity=${(parent.getChildAt(0).parent as? ViewGroup)?.let { "" }}$lit\n")

        controls.close()
    }

    @Test fun default_no_cutout() = run(Variant("default"))
    @Test fun default_sym_cutout() = run(Variant("sym-cutout", cutout = Rect(90, 0, 90, 0)))
    @Test fun default_asym_cutout() = run(Variant("asym-cutout", cutout = Rect(140, 0, 0, 0)))
    @Test fun cutout_off_asym() = run(Variant("cutout-off", avoidCutout = false, cutout = Rect(140, 0, 0, 0)))
    @Test fun scale_min() = run(Variant("scale-0.8", scale = 0.8f))
    @Test fun scale_min_asym_cutout() = run(Variant("scale-0.8-asym", scale = 0.8f, cutout = Rect(140, 0, 0, 0)))
    @Test fun scale_max() = run(Variant("scale-1.6", scale = 1.6f))
    @Test fun margins_half() = run(Variant("margins-0.5", marginX = 0.5f, marginY = 0.5f))
    @Test fun opacity_half() = run(Variant("opacity-0.5", opacity = 0.5f))
    @Test fun tablet_max() = run(Variant("tablet-1.6", scale = 1.6f, width = 2048, height = 1536, density = 2.0f))

    private fun writePng(file: File, px: IntArray, w: Int, h: Int) {
        val raw = ByteArray(h * (1 + w * 4))
        var o = 0
        for (y in 0 until h) {
            raw[o++] = 0
            for (x in 0 until w) {
                val p = px[y * w + x]
                raw[o++] = (p shr 16).toByte(); raw[o++] = (p shr 8).toByte()
                raw[o++] = p.toByte(); raw[o++] = (p ushr 24).toByte()
            }
        }
        val def = Deflater(6)
        def.setInput(raw); def.finish()
        val comp = java.io.ByteArrayOutputStream()
        val buf = ByteArray(65536)
        while (!def.finished()) comp.write(buf, 0, def.deflate(buf))
        def.end()
        fun chunk(type: String, data: ByteArray): ByteArray {
            val out = java.io.ByteArrayOutputStream()
            out.write(byteArrayOf((data.size shr 24).toByte(), (data.size shr 16).toByte(), (data.size shr 8).toByte(), data.size.toByte()))
            val td = type.toByteArray() + data
            out.write(td)
            val crc = java.util.zip.CRC32(); crc.update(td)
            val c = crc.value.toInt()
            out.write(byteArrayOf((c shr 24).toByte(), (c shr 16).toByte(), (c shr 8).toByte(), c.toByte()))
            return out.toByteArray()
        }
        val ihdr = byteArrayOf((w shr 24).toByte(), (w shr 16).toByte(), (w shr 8).toByte(), w.toByte(),
            (h shr 24).toByte(), (h shr 16).toByte(), (h shr 8).toByte(), h.toByte(), 8, 6, 0, 0, 0)
        file.writeBytes(byteArrayOf(-119, 80, 78, 71, 13, 10, 26, 10) +
            chunk("IHDR", ihdr) + chunk("IDAT", comp.toByteArray()) + chunk("IEND", byteArrayOf()))
    }
}
