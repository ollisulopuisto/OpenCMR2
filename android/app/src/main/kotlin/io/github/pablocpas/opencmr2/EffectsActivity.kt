package io.github.pablocpas.opencmr2

import android.os.Bundle
import androidx.activity.ComponentActivity
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
    private var lines by mutableStateOf(EffectSettings.DEFAULT)
    private var trails by mutableStateOf(EffectSettings.DEFAULT)
    private var fog by mutableStateOf(EffectSettings.DEFAULT)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        tunnel = EffectSettings.tunnelVision(this)
        lines = EffectSettings.tunnelLines(this)
        trails = EffectSettings.vaporTrails(this)
        fog = EffectSettings.fog(this)
        setContent {
            SetupTheme {
                Box(Modifier.fillMaxSize()) {
                    Column(Modifier.fillMaxSize().safeDrawingPadding().verticalScroll(rememberScrollState()).padding(24.dp),
                        verticalArrangement = Arrangement.spacedBy(12.dp)) {
                        Text(stringResource(R.string.effects_title), style = MaterialTheme.typography.headlineMedium)
                        Text(stringResource(R.string.effects_intro))
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Switch(checked = tunnel != null, onCheckedChange = {
                                tunnel = if (it) 130 else null
                                EffectSettings.tunnelVision(this@EffectsActivity, tunnel)
                            })
                            Text(
                                stringResource(R.string.effects_tunnel) + " · " +
                                    (tunnel?.toString() ?: stringResource(R.string.effects_state_default)),
                                Modifier.padding(start = 12.dp),
                            )
                        }
                        tunnel?.let { value ->
                            Slider(
                                value = value.toFloat(),
                                onValueChange = {
                                    tunnel = it.toInt()
                                    EffectSettings.tunnelVision(this@EffectsActivity, tunnel)
                                },
                                valueRange = 0f..200f,
                            )
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
                        Button(onClick = { finish() }) { Text(stringResource(R.string.controllers_done)) }
                    }
                }
            }
        }
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
