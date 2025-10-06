package com.rifsxd.ksunext.ui.viewmodel

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.ViewModel

enum class FlashingStatus {
    WAITING,
    FLASHING,
    SUCCESS,
    FAILED
}

class FlashViewModel : ViewModel() {
    var flashingStatus by mutableStateOf(FlashingStatus.WAITING)
        private set

    fun updateFlashingStatus(status: FlashingStatus) {
        flashingStatus = status
    }

    fun resetFlashingStatus() {
        flashingStatus = FlashingStatus.WAITING
    }
}
