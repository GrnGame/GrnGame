# Android Build Guide

## Requirements

- Make sure **Android SDK** and **Gradle** are installed on your system.
- If the build fails, edit the `local.properties` file to set your Android SDK path.

## App Settings

- **App Icon:** You can find and change it in `app/src/main/res`.
- **App Name & Properties:** Edit the files in `app/src/main/res/values`.

## Release Key

Before building the release version, you need to create a signing key.

Run this command in your terminal:

```bash
keytool -genkeypair -v -keystore release-key.jks -alias grngame -keyalg RSA -keysize 2048 -validity 10000
```

And edit app/src/build.gradle to set your password key .

Now you can build with : ./gradlew assembleDebug (no key needed) or ./gradlew assembleRelease (for publishing)
