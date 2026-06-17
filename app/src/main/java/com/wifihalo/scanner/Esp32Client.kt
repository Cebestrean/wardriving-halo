package com.wifihalo.scanner

import com.wifihalo.data.WifiNetwork
import okhttp3.OkHttpClient
import okhttp3.Request
import org.json.JSONObject
import java.util.concurrent.TimeUnit

class Esp32Client(private val host: String = "192.168.4.1") {

    private val http = OkHttpClient.Builder()
        .connectTimeout(3, TimeUnit.SECONDS)
        .readTimeout(4, TimeUnit.SECONDS)
        .build()

    fun fetchNetworks(): List<WifiNetwork> {
        val body = get("/data/aircraft.json") ?: return emptyList()
        return try {
            val json = JSONObject(body)
            val arr = json.getJSONArray("aircraft")
            (0 until arr.length()).map { i ->
                val obj = arr.getJSONObject(i)
                WifiNetwork(
                    bssid   = obj.optString("hex"),
                    ssid    = obj.optString("flight").trim(),
                    rssi    = obj.optInt("alt_baro", -100),
                    channel = obj.optInt("gs", 0)
                )
            }
        } catch (e: Exception) {
            emptyList()
        }
    }

    fun fetchBattery(): Int {
        val body = get("/battery") ?: return -1
        return try {
            JSONObject(body).optInt("level", -1)
        } catch (e: Exception) {
            -1
        }
    }

    private fun get(path: String): String? = try {
        val req = Request.Builder().url("http://$host$path").build()
        http.newCall(req).execute().use { it.body?.string() }
    } catch (e: Exception) {
        null
    }
}
