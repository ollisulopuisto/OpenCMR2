package io.github.pablocpas.opencmr2

import android.graphics.Bitmap
import androidx.compose.ui.graphics.asAndroidBitmap
import androidx.compose.ui.test.captureToImage
import androidx.compose.ui.test.isToggleable
import androidx.compose.ui.test.junit4.createAndroidComposeRule
import androidx.compose.ui.test.onNodeWithText
import androidx.compose.ui.test.onRoot
import androidx.compose.ui.test.performClick
import androidx.compose.ui.test.performScrollTo
import java.io.File
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config
import org.robolectric.annotation.GraphicsMode

/**
 * Renders the effects screen under Robolectric's native graphics and saves a
 * PNG (android/app/screenshots/) that CI uploads, so the layout is reviewed
 * as pixels, not just compiled. Not a substitute for on-device rendering.
 */
@RunWith(RobolectricTestRunner::class)
@Config(sdk = [35])
@GraphicsMode(GraphicsMode.Mode.NATIVE)
class EffectsScreenshotTest {
    @get:Rule
    val rule = createAndroidComposeRule<EffectsActivity>()

    @Test
    fun effectsScreenRenders() {
        rule.waitForIdle()
        capture("effects.png")
    }

    @Test
    fun tunnelSliderAppearsWhenEnabled() {
        rule.waitForIdle()
        rule.onAllNodes(isToggleable())[0].performClick()
        rule.waitForIdle()
        capture("effects-slider.png")
    }

    @Test
    fun scrolledStateShowsFogAndDone() {
        rule.waitForIdle()
        // Scroll the Done button into view: the Fog row then sits above it,
        // so one capture covers both. Scrolling to "Fog" alone leaves the
        // label at the frame's bottom edge with the group and Done below it.
        rule.onNodeWithText("Done").performScrollTo()
        rule.waitForIdle()
        capture("effects-scrolled.png")
    }

    private fun capture(name: String) {
        val image = rule.onRoot().captureToImage()
        val dir = File(System.getProperty("user.dir"), "screenshots").apply { mkdirs() }
        File(dir, name).outputStream().use { out ->
            image.asAndroidBitmap().compress(Bitmap.CompressFormat.PNG, 100, out)
        }
    }
}
