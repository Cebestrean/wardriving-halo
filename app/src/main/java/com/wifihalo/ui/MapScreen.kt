package com.wifihalo.ui

import android.content.Context
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.viewinterop.AndroidView
import com.wifihalo.data.WifiNetwork
import com.wifihalo.viewmodel.HaloViewModel
import org.osmdroid.tileprovider.tilesource.TileSourceFactory
import org.osmdroid.util.GeoPoint
import org.osmdroid.views.MapView
import org.osmdroid.views.overlay.Marker

@Composable
fun MapScreen(vm: HaloViewModel) {
    val networks  by vm.networks.collectAsState()
    val location  by vm.location.collectAsState()

    Box(Modifier.fillMaxSize().background(Color(0xFF0A0A0A))) {
        AndroidView(
            factory = { ctx -> buildMap(ctx) },
            update  = { map -> updateMap(map, networks, location) },
            modifier = Modifier.fillMaxSize()
        )
    }
}

private fun buildMap(ctx: Context): MapView {
    val map = MapView(ctx)
    map.setTileSource(TileSourceFactory.MAPNIK)
    map.setMultiTouchControls(true)
    map.controller.setZoom(15.0)
    map.controller.setCenter(GeoPoint(0.0, 0.0))
    return map
}

private fun updateMap(map: MapView, networks: List<WifiNetwork>, location: android.location.Location?) {
    map.overlays.removeAll { it is Marker }

    location?.let {
        val center = GeoPoint(it.latitude, it.longitude)
        map.controller.setCenter(center)
        val me = Marker(map).apply {
            position = center
            title = "You"
            setAnchor(Marker.ANCHOR_CENTER, Marker.ANCHOR_BOTTOM)
        }
        map.overlays.add(me)
    }

    map.invalidate()
}
