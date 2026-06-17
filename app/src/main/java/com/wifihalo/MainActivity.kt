package com.wifihalo

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.viewModels
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Explore
import androidx.compose.material.icons.filled.Map
import androidx.compose.material.icons.filled.Wifi
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import com.wifihalo.ui.CompassScreen
import com.wifihalo.ui.MapScreen
import com.wifihalo.ui.ScannerScreen
import com.wifihalo.viewmodel.HaloViewModel

class MainActivity : ComponentActivity() {

    private val vm: HaloViewModel by viewModels()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent { HaloApp(vm) }
    }
}

@Composable
private fun HaloApp(vm: HaloViewModel) {
    var selectedTab by remember { mutableIntStateOf(0) }

    val tabs = listOf(
        TabItem("Scan",    Icons.Filled.Wifi),
        TabItem("Compass", Icons.Filled.Explore),
        TabItem("Map",     Icons.Filled.Map)
    )

    Scaffold(
        bottomBar = {
            NavigationBar(containerColor = Color(0xFF111111)) {
                tabs.forEachIndexed { i, tab ->
                    NavigationBarItem(
                        selected = selectedTab == i,
                        onClick  = { selectedTab = i },
                        icon     = { Icon(tab.icon, contentDescription = tab.label) },
                        label    = { Text(tab.label) },
                        colors   = NavigationBarItemDefaults.colors(
                            selectedIconColor   = Color(0xFF00FF88),
                            selectedTextColor   = Color(0xFF00FF88),
                            unselectedIconColor = Color(0xFF666666),
                            unselectedTextColor = Color(0xFF666666),
                            indicatorColor      = Color(0xFF1A1A1A)
                        )
                    )
                }
            }
        },
        containerColor = Color(0xFF0A0A0A)
    ) { padding ->
        Box(Modifier.fillMaxSize().padding(padding)) {
            when (selectedTab) {
                0 -> ScannerScreen(vm)
                1 -> CompassScreen(vm)
                2 -> MapScreen(vm)
            }
        }
    }
}

private data class TabItem(val label: String, val icon: ImageVector)
