package com.rifsxd.ksunext.ui.component

import androidx.compose.foundation.Image
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.ElevatedCard
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.scale
import androidx.compose.ui.res.colorResource
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.*
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextDecoration
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.window.Dialog
import com.rifsxd.ksunext.BuildConfig
import com.rifsxd.ksunext.R
import com.rifsxd.ksunext.ui.theme.getCardElevation

@Preview
@Composable
fun AboutCard() {
    ElevatedCard(
        modifier = Modifier
            .fillMaxWidth(),
        elevation = getCardElevation()
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(24.dp)
        ) {
            AboutCardContent()
        }
    }
}

@Composable
fun AboutDialog(dismiss: () -> Unit) {
    Dialog(
        onDismissRequest = { dismiss() }
    ) {
        AboutCard()
    }
}

@Composable
private fun AboutCardContent() {
    Column(
        modifier = Modifier.fillMaxWidth()
    ) {
        Row {
            Surface(
                modifier = Modifier.size(40.dp),
                color = colorResource(id = R.color.ic_launcher_background),
                shape = CircleShape
            ) {
                Image(
                    painter = painterResource(id = R.mipmap.ic_launcher_foreground),
                    contentDescription = "icon",
                    modifier = Modifier.scale(1.5f)
                )
            }

            Spacer(modifier = Modifier.width(12.dp))

            Column {

                Text(
                    text = stringResource(id = R.string.app_name),
                    style = MaterialTheme.typography.titleMedium,
                    fontWeight = FontWeight.SemiBold,
                    fontSize = 18.sp
                )
                Text(
                    BuildConfig.VERSION_NAME,
                    style = MaterialTheme.typography.bodySmall,
                    fontSize = 14.sp
                )

                Spacer(modifier = Modifier.height(8.dp))

                val annotatedString = AnnotatedString.fromHtml(
                    htmlString = stringResource(
                        id = R.string.about_source_code,
                        "<b><a href=\"https://github.com/WildKernels/Wild_KSU\">GitHub</a></b>"
                    ),
                    linkStyles = TextLinkStyles(
                        style = SpanStyle(
                            color = MaterialTheme.colorScheme.primary,
                            textDecoration = TextDecoration.Underline
                        ),
                        pressedStyle = SpanStyle(
                            color = MaterialTheme.colorScheme.primary,
                            background = MaterialTheme.colorScheme.secondaryContainer,
                            textDecoration = TextDecoration.Underline
                        )
                    )
                )
                Text(
                    text = annotatedString,
                    style = TextStyle(
                        fontSize = 14.sp
                    )
                )
            }
        }
    }
}
