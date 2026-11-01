package io.github.pablocpas.opencmr2

import org.junit.Assert.assertEquals
import org.junit.Test

class EffectSettingsTest {
    @Test
    fun defaultsPassNoFlags() {
        assertEquals(
            emptyList<String>(),
            EffectSettings.arguments(null, null, EffectSettings.DEFAULT, EffectSettings.DEFAULT, EffectSettings.DEFAULT),
        )
    }

    @Test
    fun tunnelVisionOverride() {
        assertEquals(
            listOf("--tunnel-vision", "170"),
            EffectSettings.arguments(170, null, EffectSettings.DEFAULT, EffectSettings.DEFAULT, EffectSettings.DEFAULT),
        )
    }

    @Test
    fun tunnelVisionZeroIsAValueNotDefault() {
        assertEquals(
            listOf("--tunnel-vision", "0"),
            EffectSettings.arguments(0, null, EffectSettings.DEFAULT, EffectSettings.DEFAULT, EffectSettings.DEFAULT),
        )
    }

    @Test
    fun togglesMapToFlags() {
        assertEquals(
            listOf("--speed-fov", "40", "--tunnel-lines", "1", "--effect", "vapor_trails=off", "--effect", "fog=on"),
            EffectSettings.arguments(null, 40, EffectSettings.ON, EffectSettings.OFF, EffectSettings.ON),
        )
    }
}
