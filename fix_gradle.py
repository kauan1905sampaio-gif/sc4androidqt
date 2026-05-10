#!/usr/bin/env python3
"""
fix_gradle.py — Rode este script após cada build do Qt Creator.
Corrige o build.gradle e cria o AndroidManifest.xml no lugar correto.

Uso: python fix_gradle.py
"""
import os, glob, re, sys, stat, shutil

MANIFEST_CONTENT = '''<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="com.sceditor.app">

    <uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE"
        android:maxSdkVersion="32"/>
    <uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE"
        android:maxSdkVersion="29"/>
    <uses-permission android:name="android.permission.MANAGE_EXTERNAL_STORAGE"/>

    <application
        android:name="org.qtproject.qt.android.bindings.QtApplication"
        android:label="SC Editor"
        android:allowBackup="true"
        android:hardwareAccelerated="true"
        android:requestLegacyExternalStorage="true">

        <activity
            android:name="org.qtproject.qt.android.bindings.QtActivity"
            android:exported="true"
            android:screenOrientation="portrait"
            android:configChanges="density|fontScale|keyboard|keyboardHidden|orientation|screenSize|uiMode">
            <intent-filter>
                <action android:name="android.intent.action.MAIN"/>
                <category android:name="android.intent.category.LAUNCHER"/>
            </intent-filter>
            <intent-filter>
                <action android:name="android.intent.action.VIEW"/>
                <category android:name="android.intent.category.DEFAULT"/>
                <data android:mimeType="application/octet-stream"/>
            </intent-filter>
            <meta-data android:name="android.app.lib_name" android:value="SCEditor"/>
        </activity>
    </application>
</manifest>
'''

def fix_gradle(path):
    try:
        os.chmod(path, stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP | stat.S_IROTH)
    except:
        pass

    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()

    content = re.sub(
        r"classpath ['\"]com\.android\.tools\.build:gradle:[^'\"]*['\"]",
        "classpath 'com.android.tools.build:gradle:8.3.0'",
        content
    )
    content = re.sub(
        r"\s*android\.bundle\.enableUncompressedNativeLibs\s*=\s*(true|false)\s*\n?",
        "\n", content
    )
    content = re.sub(
        r"compileSdkVersion ['\"]android-[\d.]+['\"]",
        "compileSdkVersion 35", content
    )
    content = content.replace(
        "apply plugin: 'com.android.internal.application'",
        "apply plugin: 'com.android.application'"
    )
    content = content.replace(
        'apply plugin: "com.android.internal.application"',
        'apply plugin: "com.android.application"'
    )

    if 'apply plugin' not in content:
        content = content.rstrip() + """

apply plugin: 'com.android.application'

android {
    compileSdkVersion 35
    buildToolsVersion "35.0.0"
    namespace "com.sceditor.app"

    defaultConfig {
        applicationId "com.sceditor.app"
        minSdkVersion 26
        targetSdkVersion 35
        versionCode 1
        versionName "1.0.0"
    }

    buildTypes {
        release { minifyEnabled false }
        debug   { jniDebuggable true }
    }

    compileOptions {
        sourceCompatibility JavaVersion.VERSION_17
        targetCompatibility JavaVersion.VERSION_17
    }
}
"""

    with open(path, 'w', encoding='utf-8') as f:
        f.write(content)

    os.chmod(path, stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH)
    print(f"✓ build.gradle corrigido: {path}")

    # Create src/main/AndroidManifest.xml if missing
    build_dir = os.path.dirname(path)
    manifest_dir = os.path.join(build_dir, 'src', 'main')
    manifest_path = os.path.join(manifest_dir, 'AndroidManifest.xml')
    os.makedirs(manifest_dir, exist_ok=True)
    if not os.path.exists(manifest_path):
        with open(manifest_path, 'w', encoding='utf-8') as f:
            f.write(MANIFEST_CONTENT)
        print(f"✓ AndroidManifest.xml criado: {manifest_path}")
    else:
        print(f"  AndroidManifest.xml já existe: {manifest_path}")


base = os.path.dirname(os.path.abspath(__file__))
pattern = os.path.join(base, 'build', '*', 'android-build-*', 'build.gradle')
files = glob.glob(pattern)

if not files:
    print("Nenhum build.gradle encontrado.")
    print("Compile o projeto no Qt Creator primeiro, depois rode este script.")
    sys.exit(1)

for f in files:
    fix_gradle(f)

print("\nPronto! Agora compile novamente no Qt Creator.")
