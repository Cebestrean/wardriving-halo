package com.wifihalo.ui

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.rotate
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.wifihalo.viewmodel.HaloViewModel
import kotlin.math.cos
import kotlin.math.sin

@Composable
fun CompassScreen(vm: HaloViewModel) {
    val bearing   by vm.compass.collectAsState()
    val accuracy  by vm.compassAccuracy.collectAsState()

    val cardinalColor = Color(0xFF00FF88)
    val needleColor   = Color(0xFFFF3333)
    val ringColor     = Color(0xFF333333)

    Column(
        Modifier.fillMaxSize().background(Color(0xFF0A0A0A)),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center
    ) {
        Text(
            "COMPASS",
            color = Color(0xFF444444),
            fontSize = 11.sp,
            fontFamily = FontFamily.Monospace,
            letterSpacing = 4.sp
        )
        Spacer(Modifier.height(24.dp))

        Box(Modifier.size(280.dp), contentAlignment = Alignment.Center) {
            Canvas(Modifier.fillMaxSize()) {
                val cx = size.width / 2f
                val cy = size.height / 2f
                val r  = size.minDimension / 2f - 8f

                // Outer ring
                drawCircle(ringColor, r, Offset(cx, cy), style = androidx.compose.ui.graphics.drawscope.Stroke(2f))

                // Tick marks
                for (i in 0 until 360 step 5) {
                    val rad     = Math.toRadians(i.toDouble())
                    val tickLen = if (i % 90 == 0) 20f else if (i % 45 == 0) 14f else 8f
                    val x1 = cx + (r - tickLen) * sin(rad).toFloat()
                    val y1 = cy - (r - tickLen) * cos(rad).toFloat()
                    val x2 = cx + r * sin(rad).toFloat()
                    val y2 = cy - r * cos(rad).toFloat()
                    drawLine(if (i % 90 == 0) cardinalColor else Color(0xFF444444), Offset(x1, y1), Offset(x2, y2), 2f)
                }

                // Rotating needle
                rotate(-bearing, Offset(cx, cy)) {
                    // North needle (red)
                    drawLine(needleColor, Offset(cx, cy), Offset(cx, cy - r * 0.65f), strokeWidth = 5f)
                    // South needle (dark)
                    drawLine(Color(0xFF555555), Offset(cx, cy), Offset(cx, cy + r * 0.45f), strokeWidth = 5f)
                }

                // Center dot
                drawCircle(Color.White, 6f, Offset(cx, cy))
            }

            // Cardinal labels (fixed, compass rotates underneath)
            val labels = mapOf("N" to 0f, "E" to 90f, "S" to 180f, "W" to 270f)
            labels.forEach { (label, angle) ->
                val rad = Math.toRadians(angle.toDouble())
                val lx  = 140f + 115f * sin(rad).toFloat()
                val ly  = 140f - 115f * cos(rad).toFloat()
                Box(Modifier.offset(lx.dp - 8.dp, ly.dp - 8.dp)) {
                    Text(label, color = cardinalColor, fontSize = 13.sp, fontWeight = FontWeight.Bold)
                }
            }
        }

        Spacer(Modifier.height(32.dp))

        Text(
            "${bearing.toInt()}°",
            color = Color.White,
            fontSize = 48.sp,
            fontWeight = FontWeight.Light,
            fontFamily = FontFamily.Monospace
        )
        Text(
            bearingToCardinal(bearing),
            color = cardinalColor,
            fontSize = 16.sp,
            fontFamily = FontFamily.Monospace
        )
        Spacer(Modifier.height(12.dp))
        Text(
            accuracyLabel(accuracy),
            color = Color(0xFF666666),
            fontSize = 11.sp,
            fontFamily = FontFamily.Monospace
        )
    }
}

private fun bearingToCardinal(deg: Float): String {
    val dirs = listOf("N","NNE","NE","ENE","E","ESE","SE","SSE","S","SSW","SW","WSW","W","WNW","NW","NNW")
    return dirs[((deg + 11.25f) / 22.5f).toInt() % 16]
}

private fun accuracyLabel(acc: Int) = when (acc) {
    3    -> "HIGH ACCURACY"
    2    -> "MEDIUM ACCURACY"
    1    -> "LOW ACCURACY"
    else -> "CALIBRATING..."
}
