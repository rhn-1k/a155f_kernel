package com.rifsxd.ksunext.ui.util

import android.annotation.SuppressLint
import android.app.DownloadManager
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.net.Uri
import android.os.Environment
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.core.content.ContextCompat
import androidx.core.net.toUri
import com.rifsxd.ksunext.ksuApp
import com.rifsxd.ksunext.ui.util.module.LatestVersionInfo

/**
 * @author weishu
 * @date 2023/6/22.
 */
@SuppressLint("Range")
fun download(
    context: Context,
    url: String,
    fileName: String,
    description: String,
    onDownloaded: (Uri) -> Unit = {},
    onDownloading: () -> Unit = {}
) {
    val downloadManager = context.getSystemService(Context.DOWNLOAD_SERVICE) as DownloadManager

    val query = DownloadManager.Query()
    query.setFilterByStatus(DownloadManager.STATUS_RUNNING or DownloadManager.STATUS_PAUSED or DownloadManager.STATUS_PENDING)
    downloadManager.query(query).use { cursor ->
        while (cursor.moveToNext()) {
            val uri = cursor.getString(cursor.getColumnIndex(DownloadManager.COLUMN_URI))
            val localUri = cursor.getString(cursor.getColumnIndex(DownloadManager.COLUMN_LOCAL_URI))
            val status = cursor.getInt(cursor.getColumnIndex(DownloadManager.COLUMN_STATUS))
            val columnTitle = cursor.getString(cursor.getColumnIndex(DownloadManager.COLUMN_TITLE))
            if (url == uri || fileName == columnTitle) {
                if (status == DownloadManager.STATUS_RUNNING || status == DownloadManager.STATUS_PENDING) {
                    onDownloading()
                    return
                } else if (status == DownloadManager.STATUS_SUCCESSFUL) {
                    onDownloaded(localUri.toUri())
                    return
                }
            }
        }
    }

    val request = DownloadManager.Request(url.toUri())
        .setDestinationInExternalPublicDir(
            Environment.DIRECTORY_DOWNLOADS,
            fileName
        )
        .setNotificationVisibility(DownloadManager.Request.VISIBILITY_VISIBLE_NOTIFY_COMPLETED)
        .setMimeType("application/zip")
        .setTitle(fileName)
        .setDescription(description)

    downloadManager.enqueue(request)
}

fun checkNewVersion(): LatestVersionInfo {
    // Wild KSU version updates
    val url = "https://api.github.com/repos/WildKernels/Wild_KSU/releases/latest"
    // default null value if failed
    val defaultValue = LatestVersionInfo()
    
    // Get current manager version to determine if it's spoofed
    val context = ksuApp.applicationContext
    val currentVersionName = try {
        val packageInfo = context.packageManager.getPackageInfo(context.packageName, 0)
        packageInfo.versionName ?: ""
    } catch (e: Exception) {
        ""
    }
    val isSpoofed = currentVersionName.contains("-spoofed")
    
    runCatching {
        ksuApp.okhttpClient.newCall(okhttp3.Request.Builder().url(url).build()).execute()
            .use { response ->
                if (!response.isSuccessful) {
                    return defaultValue
                }
                val body = response.body?.string() ?: return defaultValue
                val json = org.json.JSONObject(body)
                val changelog = json.optString("body")

                val assets = json.getJSONArray("assets")
                var regularApk: Triple<Int, String, String>? = null
                var spoofedApk: Triple<Int, String, String>? = null
                
                for (i in 0 until assets.length()) {
                    val asset = assets.getJSONObject(i)
                    val name = asset.getString("name")
                    if (!name.endsWith(".apk")) {
                        continue
                    }

                    // Match APK naming format: Wild_KSU_${versionName}_${versionCode}-${buildType}.apk
                    val regex = Regex("Wild_KSU_(.+?)_(\\d+)-")
                    val matchResult = regex.find(name) ?: continue
                    val versionName = matchResult.groupValues[1]
                    val versionCode = matchResult.groupValues[2].toInt()
                    val downloadUrl = asset.getString("browser_download_url")
                    
                    if (versionName.contains("-spoofed")) {
                        spoofedApk = Triple(versionCode, downloadUrl, versionName)
                    } else {
                        regularApk = Triple(versionCode, downloadUrl, versionName)
                    }
                }
                
                // Select appropriate APK based on current manager type
                val selectedApk = if (isSpoofed) spoofedApk else regularApk
                selectedApk?.let { (versionCode, downloadUrl, _) ->
                    return LatestVersionInfo(
                        versionCode,
                        downloadUrl,
                        changelog
                    )
                }
            }
    }
    return defaultValue
}

@Composable
fun DownloadListener(context: Context, onDownloaded: (Uri) -> Unit) {
    DisposableEffect(context) {
        val receiver = object : BroadcastReceiver() {
            @SuppressLint("Range")
            override fun onReceive(context: Context?, intent: Intent?) {
                if (intent?.action == DownloadManager.ACTION_DOWNLOAD_COMPLETE) {
                    val id = intent.getLongExtra(
                        DownloadManager.EXTRA_DOWNLOAD_ID, -1
                    )
                    val query = DownloadManager.Query().setFilterById(id)
                    val downloadManager =
                        context?.getSystemService(Context.DOWNLOAD_SERVICE) as DownloadManager
                    val cursor = downloadManager.query(query)
                    if (cursor.moveToFirst()) {
                        val status = cursor.getInt(
                            cursor.getColumnIndex(DownloadManager.COLUMN_STATUS)
                        )
                        if (status == DownloadManager.STATUS_SUCCESSFUL) {
                            val uri = cursor.getString(
                                cursor.getColumnIndex(DownloadManager.COLUMN_LOCAL_URI)
                            )
                            onDownloaded(uri.toUri())
                        }
                    }
                }
            }
        }
        ContextCompat.registerReceiver(
            context,
            receiver,
            IntentFilter(DownloadManager.ACTION_DOWNLOAD_COMPLETE),
            ContextCompat.RECEIVER_EXPORTED
        )
        onDispose {
            context.unregisterReceiver(receiver)
        }
    }
}
