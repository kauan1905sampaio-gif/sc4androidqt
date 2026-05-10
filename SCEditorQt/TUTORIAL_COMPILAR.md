# Tutorial: Como compilar o SC Editor para Android

## Softwares necessários

### 1. Qt 6.11+ (com suporte Android)
- Baixe o instalador em: https://www.qt.io/download-qt-installer
- Crie uma conta Qt gratuita
- No instalador, marque:
  - Qt 6.11.x → **Android ARM64-v8a**
  - Qt 6.11.x → **Qt Creator**
  - Qt 6.11.x → **Desktop (MSVC 2022 64-bit)** *(para testar no PC)*

### 2. Android Studio
- Baixe em: https://developer.android.com/studio
- Instale normalmente
- Ao abrir pela primeira vez, instale o SDK padrão

### 3. Android SDK 35 (via Android Studio)
1. Abra o Android Studio
2. Vá em **Tools → SDK Manager → SDK Platforms**
3. Marque **Android 15 (API 35)**
4. Clique em **Apply → OK**
5. **IMPORTANTE:** NÃO instale o SDK 36.1 (causa erros no Qt)

### 4. JDK 17
- Geralmente já instalado com o Android Studio
- Se não tiver: https://adoptium.net → Temurin 17 LTS
- Marque **Add to PATH** durante instalação

### 5. Python 3
- Baixe em: https://www.python.org/downloads
- Marque **Add Python to PATH** durante instalação

---

## Configurar o Qt Creator para Android

1. Abra o Qt Creator
2. Vá em **Edit → Preferences → SDKs → Android**
3. Preencha:
   - **JDK location:** pasta onde o JDK 17 foi instalado
   - **Android SDK location:** `C:\AndroidSdk`
4. O NDK será detectado automaticamente
5. Clique em **Apply**
6. Confirme que aparece **Android settings are OK**

---

## Compilar o projeto

### Passo 1 — Abrir o projeto
1. Qt Creator: **File → Open File or Project**
2. Selecione `CMakeLists.txt` dentro da pasta `SCEditorQt`
3. Selecione o kit **Qt 6.11 for Android ARM64-v8a**
4. Clique em **Configure Project**

### Passo 2 — Testar no Desktop primeiro
1. Selecione o kit **Desktop Qt 6.11 MSVC2022** no seletor inferior esquerdo
2. Clique **▶ Run** (Ctrl+R)
3. O app deve abrir no Windows — confirme que funciona

### Passo 3 — Compilar para Android
1. Selecione o kit **Qt 6.11 for Android ARM64-v8a**
2. Em **Projects → Build Settings → Build Android APK**:
   - **Android build-tools version:** 35.0.0
   - **Android build platform SDK:** android-35
3. Compile: **Build → Build Project** (Ctrl+B)
4. **Se falhar**, rode no Prompt de Comando:
   ```
   python fix_gradle.py
   ```
5. Compile novamente — ficará verde

### Passo 4 — Gerar APK com tamanho correto (Release)
1. Em **Projects → Build & Run**, crie uma configuração Release:
   - Clique em **Add → Release**
   - Ou **Clone** do Debug e mude Build type para Release
2. Selecione **Release** no seletor
3. Em Build Settings ajuste:
   - Android build-tools: 35.0.0
   - Android build platform SDK: android-35
4. Compile

O APK Release ficará em:
```
build\Qt_6_11_0_for_Android_arm64_v8a-Release\android-build-SCEditor\build\outputs\apk\release\
```

### Passo 5 — Instalar no celular (sem USB)
1. Faça upload do APK para o **Google Drive** ou **Telegram**
2. No celular, baixe e abra o arquivo
3. Se aparecer "fontes desconhecidas":
   - **Configurações → Segurança → Instalar apps desconhecidos**
   - Permita para o app que fez o download

---

## O script fix_gradle.py

O Qt gera um `build.gradle` com configurações incompatíveis. O script corrige automaticamente:
- Versão do plugin Android Gradle
- SDK version (remove o formato android-36.1)
- Plugin interno → plugin público
- Cria `src/main/AndroidManifest.xml` se ausente

**Quando rodar:** sempre que o build falhar com erros de Gradle.

```
cd C:\Users\SeuUsuario\Downloads\SCEditorQt\SCEditorQt
python fix_gradle.py
```

---

## Problemas comuns

| Erro | Solução |
|------|---------|
| `android-36.1 does not exist` | Rode `python fix_gradle.py` |
| `assembleDebug/Release not found` | Rode `python fix_gradle.py` |
| `com.android.internal.application` | Rode `python fix_gradle.py` |
| `AndroidManifest.xml not found` | Rode `python fix_gradle.py` |
| `Can't remove old file: build.gradle` | Remova o atributo somente leitura do build.gradle |
| APK ficou 3KB | Verifique Android build platform SDK = android-35 |
| App não abre no celular | Habilite "fontes desconhecidas" nas configurações |

---

## Estrutura do projeto

```
SCEditorQt/
├── CMakeLists.txt           ← configuração de build
├── fix_gradle.py            ← script corretor do Gradle
├── android/
│   ├── AndroidManifest.xml  ← permissões Android
│   ├── build.gradle         ← configuração Gradle base
│   └── gradle.properties    ← propriedades SDK
└── src/
    ├── main.cpp
    ├── ScFile.h             ← modelos de dados
    ├── ScParser.h/cpp       ← parser SC v0/v3/v4
    ├── Decompressor.h/cpp   ← LZMA/ZSTD/LZHAM
    ├── ScCombiner.h/cpp     ← combinar arquivos SC
    ├── MainWindow.h/cpp     ← janela principal
    ├── ViewerWidget.h/cpp   ← Exports/Shapes/Textures/Matrices/Info
    ├── EditorWidget.h/cpp   ← editor frames + posição
    └── PreviewWidget.h/cpp  ← preview animação QPainter
```
