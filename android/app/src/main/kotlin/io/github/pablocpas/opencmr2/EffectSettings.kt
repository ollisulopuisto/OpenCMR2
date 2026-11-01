package io.github.pablocpas.opencmr2

import android.content.Context

/**
 * Graphics-effect choices from the effects screen. Stored in their own
 * SharedPreferences and passed to the game as command-line flags in
 * GameActivity.getArguments: the game's option system lets CLI flags win
 * over opencmr2.ini (src/platform/sys.cpp Sys_GetOption), so the ini and
 * the desktop defaults stay untouched. Every setting has a Default state
 * that passes no flag. Flags are launch arguments, so a change applies the
 * next time the game starts - which is every launch, since the launcher
 * reopens when the game exits.
 *
 * 3D trees and animated spectators are deliberately NOT here: they need a
 * prepared scenery pack, and preparing one requires the desktop build
 * (src/platform/sys.cpp runs Python tooling for it). Passing --trees
 * without a pack stops the game from starting, so those stay desktop-only
 * until a pack delivery route exists on Android.
 */
object EffectSettings {
    /** Tri-state toggle values. DEFAULT passes no flag. */
    const val DEFAULT = 0
    const val ON = 1
    const val OFF = 2

    private const val PREFS = "effects"
    private const val KEY_TUNNEL_VISION = "tunnel_vision"
    private const val KEY_TUNNEL_LINES = "tunnel_lines"
    private const val KEY_VAPOR_TRAILS = "vapor_trails"
    private const val KEY_FOG = "fog"
    private const val UNSET = -1

    private fun prefs(context: Context) = context.getSharedPreferences(PREFS, Context.MODE_PRIVATE)

    /** Tunnel vision strength 0..200, or null for the game's own default. */
    fun tunnelVision(context: Context): Int? =
        prefs(context).getInt(KEY_TUNNEL_VISION, UNSET).takeIf { it in 0..200 }

    fun tunnelVision(context: Context, value: Int?) {
        prefs(context).edit().putInt(KEY_TUNNEL_VISION, value?.coerceIn(0, 200) ?: UNSET).apply()
    }

    private fun toggle(context: Context, key: String): Int =
        prefs(context).getInt(key, DEFAULT).takeIf { it in DEFAULT..OFF } ?: DEFAULT

    private fun setToggle(context: Context, key: String, value: Int) {
        prefs(context).edit().putInt(key, value.coerceIn(DEFAULT, OFF)).apply()
    }

    fun tunnelLines(context: Context) = toggle(context, KEY_TUNNEL_LINES)
    fun tunnelLines(context: Context, value: Int) = setToggle(context, KEY_TUNNEL_LINES, value)
    fun vaporTrails(context: Context) = toggle(context, KEY_VAPOR_TRAILS)
    fun vaporTrails(context: Context, value: Int) = setToggle(context, KEY_VAPOR_TRAILS, value)
    fun fog(context: Context) = toggle(context, KEY_FOG)
    fun fog(context: Context, value: Int) = setToggle(context, KEY_FOG, value)

    /**
     * The game flags for the given values, in the syntax parsed by
     * src/platform/enhancements.h. Pure, so unit tests need no Android runtime.
     */
    fun arguments(tunnelVision: Int?, tunnelLines: Int, vaporTrails: Int, fog: Int): List<String> {
        val args = mutableListOf<String>()
        tunnelVision?.let { args += listOf("--tunnel-vision", it.toString()) }
        when (tunnelLines) {
            ON -> args += listOf("--tunnel-lines", "1")
            OFF -> args += listOf("--tunnel-lines", "0")
        }
        when (vaporTrails) {
            ON -> args += listOf("--effect", "vapor_trails=on")
            OFF -> args += listOf("--effect", "vapor_trails=off")
        }
        when (fog) {
            ON -> args += listOf("--effect", "fog=on")
            OFF -> args += listOf("--effect", "fog=off")
        }
        return args
    }

    fun arguments(context: Context): List<String> =
        arguments(tunnelVision(context), tunnelLines(context), vaporTrails(context), fog(context))
}
