package com.wifihalo.data

data class WifiNetwork(
    val bssid: String,
    val ssid: String,
    val rssi: Int,
    val channel: Int,
    val seenAt: Long = System.currentTimeMillis()
)
