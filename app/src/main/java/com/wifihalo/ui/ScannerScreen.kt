package com.wifihalo.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Wifi
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.wifihalo.data.WifiNetwork
import com.wifihalo.viewmodel.HaloViewModel

@Composable
fun ScannerScreen(vm: HaloViewModel) {
    val networks by vm.networks.collectAsState()
    val battery  by vm.battery.collectAsState()
    val connected by vm.esp32Connected.collectAsState()

    Column(Modifier.fillMaxSize().background(Color(0xFF0A0A0A))) {
        // Status bar
        Row(
            Modifier.fillMaxWidth().background(Color(0xFF111111)).padding(8.dp),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Icon(
                    Icons.Filled.Wifi,
                    contentDescription = null,
                    tint = if (connected) Color(0xFF00FF88) else Color(0xFF666666),
                    modifier = Modifier.size(16.dp)
                )
                Spacer(Modifier.width(6.dp))
                Text(
                    if (connected) "ESP32 Connected" else "ESP32 Not Found",
                    color = if (connected) Color(0xFF00FF88) else Color(0xFF888888),
                    fontSize = 12.sp
                )
            }
            Text(
                if (battery >= 0) "BAT $battery%" else "BAT --",
                color = Color(0xFF888888),
                fontSize = 12.sp,
                fontFamily = FontFamily.Monospace
            )
        }

        // Network count
        Text(
            "${networks.size} networks",
            color = Color(0xFF00FF88),
            fontSize = 11.sp,
            fontFamily = FontFamily.Monospace,
            modifier = Modifier.padding(horizontal = 12.dp, vertical = 4.dp)
        )

        if (networks.isEmpty()) {
            Box(Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                Text(
                    if (connected) "Scanning..." else "Connect to WardrivingHalo WiFi AP",
                    color = Color(0xFF444444),
                    fontSize = 14.sp
                )
            }
        } else {
            LazyColumn(Modifier.fillMaxSize()) {
                items(networks.sortedByDescending { it.rssi }) { net ->
                    NetworkRow(net)
                    Divider(color = Color(0xFF1A1A1A), thickness = 0.5.dp)
                }
            }
        }
    }
}

@Composable
private fun NetworkRow(net: WifiNetwork) {
    val signalColor = when {
        net.rssi >= -50 -> Color(0xFF00FF88)
        net.rssi >= -70 -> Color(0xFFFFCC00)
        else            -> Color(0xFFFF4444)
    }
    Row(
        Modifier.fillMaxWidth().padding(horizontal = 12.dp, vertical = 8.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Column(Modifier.weight(1f)) {
            Text(
                net.ssid.ifBlank { "<hidden>" },
                color = Color.White,
                fontSize = 14.sp
            )
            Text(
                net.bssid,
                color = Color(0xFF888888),
                fontSize = 11.sp,
                fontFamily = FontFamily.Monospace
            )
        }
        Column(horizontalAlignment = Alignment.End) {
            Text(
                "${net.rssi} dBm",
                color = signalColor,
                fontSize = 12.sp,
                fontFamily = FontFamily.Monospace
            )
            Text(
                "CH ${net.channel}",
                color = Color(0xFF666666),
                fontSize = 11.sp,
                fontFamily = FontFamily.Monospace
            )
        }
    }
}
