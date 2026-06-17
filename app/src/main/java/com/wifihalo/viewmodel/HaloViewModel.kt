package com.wifihalo.viewmodel

import android.app.Application
import android.location.Location
import android.location.LocationListener
import android.location.LocationManager
import android.content.Context
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.wifihalo.data.WifiNetwork
import com.wifihalo.scanner.Esp32Client
import com.wifihalo.sensor.CompassManager
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.launch

class HaloViewModel(app: Application) : AndroidViewModel(app) {

    private val esp32 = Esp32Client()
    private val compassMgr = CompassManager(app)

    private val _networks = MutableStateFlow<List<WifiNetwork>>(emptyList())
    val networks: StateFlow<List<WifiNetwork>> = _networks

    private val _battery = MutableStateFlow(-1)
    val battery: StateFlow<Int> = _battery

    private val _esp32Connected = MutableStateFlow(false)
    val esp32Connected: StateFlow<Boolean> = _esp32Connected

    private val _location = MutableStateFlow<Location?>(null)
    val location: StateFlow<Location?> = _location

    val compass         = compassMgr.bearing
    val compassAccuracy = compassMgr.accuracy

    init {
        compassMgr.start()
        startPolling()
        startGps(app)
    }

    private fun startPolling() = viewModelScope.launch(Dispatchers.IO) {
        while (true) {
            val nets = esp32.fetchNetworks()
            _esp32Connected.value = nets.isNotEmpty() || esp32.fetchBattery() >= 0
            if (nets.isNotEmpty()) _networks.value = nets
            _battery.value = esp32.fetchBattery()
            delay(5_000)
        }
    }

    private fun startGps(ctx: Context) {
        try {
            val lm = ctx.getSystemService(Context.LOCATION_SERVICE) as LocationManager
            val listener = LocationListener { _location.value = it }
            lm.requestLocationUpdates(LocationManager.GPS_PROVIDER, 2000L, 1f, listener)
        } catch (_: SecurityException) {}
    }

    override fun onCleared() {
        super.onCleared()
        compassMgr.stop()
    }
}
