package com.wifihalo.sensor

import android.content.Context
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import android.os.Handler
import android.os.Looper
import android.view.Display
import android.view.Surface
import android.view.WindowManager
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow

class CompassManager(context: Context) {

    private val sensorManager = context.getSystemService(Context.SENSOR_SERVICE) as SensorManager
    private val windowManager = context.getSystemService(Context.WINDOW_SERVICE) as WindowManager

    private val _bearing = MutableStateFlow(0f)
    val bearing: StateFlow<Float> = _bearing

    private val _accuracy = MutableStateFlow(SensorManager.SENSOR_STATUS_UNRELIABLE)
    val accuracy: StateFlow<Int> = _accuracy

    private val rotMatrix  = FloatArray(9)
    private val remapped   = FloatArray(9)
    private val orientation = FloatArray(3)
    private val gravity    = FloatArray(3)
    private val geomag     = FloatArray(3)

    private val display: Display get() = windowManager.defaultDisplay

    private val listener = object : SensorEventListener {
        override fun onAccuracyChanged(sensor: Sensor, acc: Int) {
            _accuracy.value = acc
        }

        override fun onSensorChanged(event: SensorEvent) {
            when (event.sensor.type) {
                Sensor.TYPE_ROTATION_VECTOR,
                Sensor.TYPE_GEOMAGNETIC_ROTATION_VECTOR -> {
                    SensorManager.getRotationMatrixFromVector(rotMatrix, event.values)
                    remapAndPublish()
                    if (event.sensor.type == Sensor.TYPE_ROTATION_VECTOR) _accuracy.value = 3
                }
                Sensor.TYPE_ACCELEROMETER ->
                    System.arraycopy(event.values, 0, gravity, 0, 3)
                Sensor.TYPE_MAGNETIC_FIELD -> {
                    System.arraycopy(event.values, 0, geomag, 0, 3)
                    if (SensorManager.getRotationMatrix(rotMatrix, null, gravity, geomag)) {
                        remapAndPublish()
                    }
                }
            }
        }
    }

    // Fix for Pixel 8 Pro (and all devices): remap axes to match current display rotation
    // so the compass heading is correct regardless of how the phone is oriented.
    private fun remapAndPublish() {
        val (axisX, axisY) = when (display.rotation) {
            Surface.ROTATION_90  -> Pair(SensorManager.AXIS_Y, SensorManager.AXIS_MINUS_X)
            Surface.ROTATION_180 -> Pair(SensorManager.AXIS_MINUS_X, SensorManager.AXIS_MINUS_Y)
            Surface.ROTATION_270 -> Pair(SensorManager.AXIS_MINUS_Y, SensorManager.AXIS_X)
            else                 -> Pair(SensorManager.AXIS_X, SensorManager.AXIS_Y) // ROTATION_0 / portrait
        }
        SensorManager.remapCoordinateSystem(rotMatrix, axisX, axisY, remapped)
        SensorManager.getOrientation(remapped, orientation)
        _bearing.value = ((Math.toDegrees(orientation[0].toDouble()).toFloat() + 360f) % 360f)
    }

    fun start() {
        val handler = Handler(Looper.getMainLooper())
        // Prefer rotation vector (most accurate, fuses accel+gyro+mag)
        val rotVec = sensorManager.getDefaultSensor(Sensor.TYPE_ROTATION_VECTOR)
        if (rotVec != null) {
            sensorManager.registerListener(listener, rotVec, SensorManager.SENSOR_DELAY_UI, handler)
            return
        }
        // Fallback: geomagnetic rotation vector (no gyro needed)
        val geoRot = sensorManager.getDefaultSensor(Sensor.TYPE_GEOMAGNETIC_ROTATION_VECTOR)
        if (geoRot != null) {
            sensorManager.registerListener(listener, geoRot, SensorManager.SENSOR_DELAY_UI, handler)
            return
        }
        // Final fallback: raw accel + mag
        sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER)?.let {
            sensorManager.registerListener(listener, it, SensorManager.SENSOR_DELAY_UI, handler)
        }
        sensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD)?.let {
            sensorManager.registerListener(listener, it, SensorManager.SENSOR_DELAY_UI, handler)
        }
    }

    fun stop() {
        sensorManager.unregisterListener(listener)
    }
}
