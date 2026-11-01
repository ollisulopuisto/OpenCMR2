package io.github.pablocpas.opencmr2

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.lifecycle.lifecycleScope
import android.text.format.Formatter
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp

/** Launch-time graphics effects, passed to the game as flags (EffectSettings). */
class EffectsActivity : ComponentActivity() {
    private var tunnel by mutableStateOf<Int?>(null)
    private var speedFov by mutableStateOf<Int?>(null)
    private var lines by mutableStateOf(EffectSettings.DEFAULT)
    private var trails by mutableStateOf(EffectSettings.DEFAULT)
    private var fog by mutableStateOf(EffectSettings.DEFAULT)
    private var sceneryMappings by mutableStateOf(0)
    private var sceneryWorking by mutableStateOf<String?>(null)
    private var sceneryError by mutableStateOf<String?>(null)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        tunnel = EffectSettings.tunnelVision(this)
        speedFov = EffectSettings.speedFov(this)
        lines = EffectSettings.tunnelLines(this)
        trails = EffectSettings.vaporTrails(this)
        fog = EffectSettings.fog(this)
        sceneryMappings = SceneryPack.mappings(this)
        setContent {
            SetupTheme {
                Box(Modifier.fillMaxSize()) {
                    Column(Modifier.fillMaxSize().safeDrawingPadding().verticalScroll(rememberScrollState()).padding(24.dp),
                        verticalArrangement = Arrangement.spacedBy(12.dp)) {
                        Text(stringResource(R.string.effects_title), style = MaterialTheme.typography.headlineMedium)
                        Text(stringResource(R.string.effects_intro))
                        SliderOverride(R.string.effects_tunnel, tunnel, 130, 0f..200f) {
                            tunnel = it; EffectSettings.tunnelVision(this@EffectsActivity, it)
                        }
                        SliderOverride(R.string.effects_speed_fov, speedFov, 40, 0f..100f) {
                            speedFov = it; EffectSettings.speedFov(this@EffectsActivity, it)
                        }
                        EffectToggle(R.string.effects_lines, lines) {
                            lines = it; EffectSettings.tunnelLines(this@EffectsActivity, it)
                        }
                        EffectToggle(R.string.effects_trails, trails) {
                            trails = it; EffectSettings.vaporTrails(this@EffectsActivity, it)
                        }
                        EffectToggle(R.string.effects_fog, fog) {
                            fog = it; EffectSettings.fog(this@EffectsActivity, it)
                        }
                        ScenerySection(
                            mappings = sceneryMappings,
                            working = sceneryWorking,
                            error = sceneryError,
                            onImport = { tree -> importScenery(tree) },
                            onRemove = {
                                SceneryPack.remove(this)
                                sceneryMappings = 0
                                sceneryError = null
                            },
                        )
                        Button(onClick = { finish() }) { Text(stringResource(R.string.controllers_done)) }
                    }
                }
            }
        }
    }
    private fun importScenery(tree: android.net.Uri) {
        if (sceneryWorking != null) return
        sceneryError = null
        lifecycleScope.launch {
            try {
                withContext(Dispatchers.IO) {
                    SceneryPack.import(this@EffectsActivity, tree) { progress ->
                        sceneryWorking = when (progress) {
                            is SceneryPack.Progress.Searching ->
                                getString(R.string.effects_scenery_searching)
                            is SceneryPack.Progress.Copying ->
                                getString(R.string.effects_scenery_copying,
                                    progress.copied shr 20, progress.total shr 20)
                        }
                    }
                }
                sceneryMappings = SceneryPack.mappings(this@EffectsActivity)
                sceneryWorking = null
            } catch (e: Exception) {
                sceneryWorking = null
                sceneryError = when (e) {
                    is SceneryPack.ImportException.NotFound ->
                        getString(R.string.effects_scenery_no_manifest)
                    is SceneryPack.ImportException.Ambiguous ->
                        getString(R.string.effects_scenery_ambiguous, e.folders.joinToString())
                    is SceneryPack.ImportException.Unsafe ->
                        getString(R.string.effects_scenery_unsafe, e.reference)
                    is SceneryPack.ImportException.NoSpace ->
                        getString(R.string.setup_no_space,
                            Formatter.formatShortFileSize(this@EffectsActivity, e.needed),
                            Formatter.formatShortFileSize(this@EffectsActivity, e.available))
                    else -> getString(R.string.setup_failed, e.localizedMessage ?: e.toString())
                }
            }
        }
    }
}

@Composable
private fun ScenerySection(mappings: Int, working: String?, error: String?,
    onImport: (android.net.Uri) -> Unit, onRemove: () -> Unit) {
    val chooser = rememberLauncherForActivityResult(ActivityResultContracts.OpenDocumentTree()) { uri ->
        if (uri != null) onImport(uri)
    }
    Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
        Text(stringResource(R.string.effects_scenery_title))
        Text(
            if (mappings > 0) stringResource(R.string.effects_scenery_installed, mappings)
            else stringResource(R.string.effects_scenery_none),
            style = MaterialTheme.typography.bodySmall,
        )
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            OutlinedButton(onClick = { chooser.launch(null) }, enabled = working == null) {
                Text(stringResource(R.string.effects_scenery_import))
            }
            if (mappings > 0) {
                OutlinedButton(onClick = onRemove, enabled = working == null) {
                    Text(stringResource(R.string.effects_scenery_remove))
                }
            }
        }
        working?.let { Text(it, style = MaterialTheme.typography.bodySmall) }
        error?.let { Text(it, color = MaterialTheme.colorScheme.error, style = MaterialTheme.typography.bodySmall) }
    }
}

@Composable
private fun SliderOverride(label: Int, value: Int?, defaultOnEnable: Int,
    range: ClosedFloatingPointRange<Float>, onChange: (Int?) -> Unit) {
    Row(verticalAlignment = Alignment.CenterVertically) {
        Switch(checked = value != null, onCheckedChange = { onChange(if (it) defaultOnEnable else null) })
        Text(
            stringResource(label) + " · " + (value?.toString() ?: stringResource(R.string.effects_state_default)),
            Modifier.padding(start = 12.dp),
        )
    }
    value?.let {
        Slider(value = it.toFloat(), onValueChange = { v -> onChange(v.toInt()) }, valueRange = range)
    }
}

@Composable
private fun EffectToggle(label: Int, value: Int, onChange: (Int) -> Unit) {
    Column {
        Text(stringResource(label))
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            val states = listOf(
                EffectSettings.DEFAULT to stringResource(R.string.effects_state_default),
                EffectSettings.ON to stringResource(R.string.effects_on),
                EffectSettings.OFF to stringResource(R.string.effects_off),
            )
            for ((state, text) in states) {
                OutlinedButton(onClick = { onChange(state) }) {
                    Text((if (value == state) "✓ " else "") + text)
                }
            }
        }
    }
}
