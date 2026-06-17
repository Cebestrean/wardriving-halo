package com.wifihalo

import android.app.Application
import org.osmdroid.config.Configuration

class HaloApp : Application() {
    override fun onCreate() {
        super.onCreate()
        Configuration.getInstance().userAgentValue = packageName
    }
}
