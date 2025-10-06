package com.rifsxd.ksunext.ui.screen

import androidx.annotation.StringRes
import androidx.compose.animation.Crossfade
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.AccountCircle
import androidx.compose.material.icons.filled.AdminPanelSettings
import androidx.compose.material.icons.filled.Android
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset

import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.DpOffset
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.ui.unit.dp

import androidx.lifecycle.viewmodel.compose.viewModel
import coil.compose.AsyncImage
import coil.request.ImageRequest
import com.ramcosta.composedestinations.annotation.Destination
import com.ramcosta.composedestinations.annotation.RootGraph
import com.ramcosta.composedestinations.generated.destinations.AppProfileTemplateScreenDestination
import com.ramcosta.composedestinations.generated.destinations.TemplateEditorScreenDestination
import com.ramcosta.composedestinations.navigation.DestinationsNavigator
import com.rifsxd.ksunext.Natives
import com.rifsxd.ksunext.R
import com.rifsxd.ksunext.ui.component.SwitchItem
import com.rifsxd.ksunext.ui.component.ThemedIcon
import com.rifsxd.ksunext.ui.component.profile.AppProfileConfig
import com.rifsxd.ksunext.ui.component.profile.RootProfileConfig
import com.rifsxd.ksunext.ui.component.profile.TemplateConfig
import com.rifsxd.ksunext.ui.util.*
import com.rifsxd.ksunext.ui.viewmodel.SuperUserViewModel
import com.rifsxd.ksunext.ui.viewmodel.getTemplateInfoById
import com.rifsxd.ksunext.ui.LocalSuperUserViewModel
import kotlinx.coroutines.launch

/**
 * @author weishu
 * @date 2023/5/16.
 */
@OptIn(ExperimentalMaterial3Api::class)
@Destination<RootGraph>
@Composable
fun AppProfileScreen(
    navigator: DestinationsNavigator,
    appInfo: SuperUserViewModel.AppInfo,
) {
    val context = LocalContext.current
    val snackBarHost = LocalSnackbarHost.current
    val scope = rememberCoroutineScope()
    val viewModel = LocalSuperUserViewModel.current
    val failToUpdateAppProfile = stringResource(R.string.failed_to_update_app_profile).format(appInfo.label)
    val failToUpdateSepolicy = stringResource(R.string.failed_to_update_sepolicy).format(appInfo.label)
    val suNotAllowed = stringResource(R.string.su_not_allowed).format(appInfo.label)

    val packageName = appInfo.packageName
    val initialProfile = Natives.getAppProfile(packageName, appInfo.uid)
    if (initialProfile.allowSu) {
        initialProfile.rules = getSepolicy(packageName)
    }
    var profile by rememberSaveable {
        mutableStateOf(initialProfile)
    }

    AppProfileInner(
        modifier = Modifier
            .fillMaxSize()
            .verticalScroll(rememberScrollState()),
        packageName = appInfo.packageName,
        appLabel = appInfo.label,
        appIcon = {
            ThemedIcon(
                packageName = appInfo.packageName,
                packageInfo = appInfo.packageInfo,
                contentDescription = appInfo.label,
                modifier = Modifier
                    .padding(4.dp)
                    .width(48.dp)
                    .height(48.dp)
            )
        },
        profile = profile,
        onViewTemplate = {
            getTemplateInfoById(it)?.let { info ->
                navigator.navigate(TemplateEditorScreenDestination(info))
            }
        },
        onManageTemplate = {
            navigator.navigate(AppProfileTemplateScreenDestination())
        },
        onProfileChange = {
            scope.launch {
                if (it.allowSu) {
                    // sync with allowlist.c - forbid_system_uid
                    if (appInfo.uid < 2000 && appInfo.uid != 1000) {
                        snackBarHost.showSnackbar(suNotAllowed)
                        return@launch
                    }
                    if (!it.rootUseDefault && it.rules.isNotEmpty() && !setSepolicy(profile.name, it.rules)) {
                        snackBarHost.showSnackbar(failToUpdateSepolicy)
                        return@launch
                    }
                }
                if (!Natives.setAppProfile(it)) {
                    snackBarHost.showSnackbar(failToUpdateAppProfile.format(appInfo.uid))
                } else {
                    profile = it
                    viewModel.updateAppProfile(packageName, it)
                }
            }
        },
    )
}

@Composable
private fun AppProfileInner(
    modifier: Modifier = Modifier,
    packageName: String,
    appLabel: String,
    appIcon: @Composable () -> Unit,
    profile: Natives.Profile,
    onViewTemplate: (id: String) -> Unit = {},
    onManageTemplate: () -> Unit = {},
    onProfileChange: (Natives.Profile) -> Unit,
) {
    val isRootGranted = profile.allowSu

    Column(
        modifier = modifier.padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        Card(
            shape = RoundedCornerShape(12.dp),
            colors = CardDefaults.cardColors(
                containerColor = MaterialTheme.colorScheme.surfaceContainer
            )
        ) {
            AppMenuBox(packageName) {
                ListItem(
                    headlineContent = { Text(
                        text = appLabel,
                        style = MaterialTheme.typography.titleMedium,
                        fontWeight = FontWeight.SemiBold
                    ) },
                    supportingContent = { Text(packageName) },
                    leadingContent = appIcon,
                )
            }
        }

        Card(
            shape = RoundedCornerShape(12.dp),
            colors = CardDefaults.cardColors(
                containerColor = MaterialTheme.colorScheme.surfaceContainer
            )
        ) {
            SwitchItem(
                icon = Icons.Filled.AdminPanelSettings,
                title = stringResource(id = R.string.superuser),
                checked = isRootGranted,
                onCheckedChange = { onProfileChange(profile.copy(allowSu = it)) },
            )
        }

        Crossfade(targetState = isRootGranted, label = "") { current ->
            Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                if (current) {
                    val initialMode = if (profile.rootUseDefault) {
                        Mode.Default
                    } else if (profile.rootTemplate != null) {
                        Mode.Template
                    } else {
                        Mode.Custom
                    }
                    var mode by rememberSaveable {
                        mutableStateOf(initialMode)
                    }
                    Card(
                        shape = RoundedCornerShape(12.dp),
                        colors = CardDefaults.cardColors(
                            containerColor = MaterialTheme.colorScheme.surfaceContainer
                        )
                    ) {
                        ProfileBox(mode, true) {
                            // template mode shouldn't change profile here!
                            if (it == Mode.Default || it == Mode.Custom) {
                                onProfileChange(profile.copy(rootUseDefault = it == Mode.Default))
                            }
                            mode = it
                        }
                    }
                    Crossfade(targetState = mode, label = "") { currentMode ->
                        if (currentMode == Mode.Template) {
                            TemplateConfig(
                                profile = profile,
                                onViewTemplate = onViewTemplate,
                                onManageTemplate = onManageTemplate,
                                onProfileChange = onProfileChange
                            )
                        } else if (mode == Mode.Custom) {
                            RootProfileConfig(
                                fixedName = true,
                                profile = profile,
                                onProfileChange = onProfileChange
                            )
                        }
                    }
                } else {
                    val mode = if (profile.nonRootUseDefault) Mode.Default else Mode.Custom
                    Card(
                        shape = RoundedCornerShape(12.dp),
                        colors = CardDefaults.cardColors(
                            containerColor = MaterialTheme.colorScheme.surfaceContainer
                        )
                    ) {
                        ProfileBox(mode, false) {
                            onProfileChange(profile.copy(nonRootUseDefault = (it == Mode.Default)))
                        }
                    }
                    Crossfade(targetState = mode, label = "") { currentMode ->
                        val modifyEnabled = currentMode == Mode.Custom
                        AppProfileConfig(
                            fixedName = true,
                            profile = profile,
                            enabled = modifyEnabled,
                            onProfileChange = onProfileChange
                        )
                    }
                }
            }
        }
    }
}

private enum class Mode(@param:StringRes private val res: Int) {
    Default(R.string.profile_default), Template(R.string.profile_template), Custom(R.string.profile_custom);

    val text: String
        @Composable get() = stringResource(res)
}

@Composable
private fun ProfileBox(
    mode: Mode,
    hasTemplate: Boolean,
    onModeChange: (Mode) -> Unit,
) {
    ListItem(
        headlineContent = { Text(
            text = stringResource(R.string.profile),
            style = MaterialTheme.typography.titleMedium,
            fontWeight = FontWeight.SemiBold
        ) },
        supportingContent = { Text(mode.text) },
        leadingContent = { Icon(Icons.Filled.AccountCircle, null) },
    )
    HorizontalDivider(thickness = Dp.Hairline)
    ListItem(headlineContent = {
        Row(
            modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceEvenly
        ) {
            FilterChip(
                selected = mode == Mode.Default,
                label = { Text(stringResource(R.string.profile_default)) },
                onClick = { onModeChange(Mode.Default) },
            )
            if (hasTemplate) {
                FilterChip(
                    selected = mode == Mode.Template,
                    label = { Text(stringResource(R.string.profile_template)) },
                    onClick = { onModeChange(Mode.Template) },
                )
            }
            FilterChip(
                selected = mode == Mode.Custom,
                label = { Text(stringResource(R.string.profile_custom)) },
                onClick = { onModeChange(Mode.Custom) },
            )
        }
    })
}

@Composable
private fun AppMenuBox(packageName: String, content: @Composable () -> Unit) {

    var expanded by remember { mutableStateOf(false) }
    var touchPoint: Offset by remember { mutableStateOf(Offset.Zero) }
    val density = LocalDensity.current

    BoxWithConstraints(
        Modifier
            .fillMaxSize()
            .pointerInput(Unit) {
                detectTapGestures {
                    touchPoint = it
                    expanded = true
                }
            }
    ) {

        content()

        val (offsetX, offsetY) = with(density) {
            (touchPoint.x.toDp()) to (touchPoint.y.toDp())
        }

        DropdownMenu(
            expanded = expanded,
            offset = DpOffset(offsetX, -offsetY),
            onDismissRequest = {
                expanded = false
            },
        ) {
            DropdownMenuItem(
                text = { Text(stringResource(id = R.string.launch_app)) },
                onClick = {
                    expanded = false
                    launchApp(packageName)
                },
            )
            DropdownMenuItem(
                text = { Text(stringResource(id = R.string.force_stop_app)) },
                onClick = {
                    expanded = false
                    forceStopApp(packageName)
                },
            )
            DropdownMenuItem(
                text = { Text(stringResource(id = R.string.restart_app)) },
                onClick = {
                    expanded = false
                    restartApp(packageName)
                },
            )
        }
    }


}

@Preview
@Composable
private fun AppProfilePreview() {
    var profile by remember { mutableStateOf(Natives.Profile("")) }
    AppProfileInner(
        packageName = "icu.nullptr.test",
        appLabel = "Test",
        appIcon = { Icon(Icons.Filled.Android, null) },
        profile = profile,
        onProfileChange = {
            profile = it
        },
    )
}

