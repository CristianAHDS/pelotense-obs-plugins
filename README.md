# pelotense-obs-plugins

Plugins para OBS Studio desenvolvidos pelo Pelotense.

## Plugins

- **PelotenseAssets** — gerenciamento de assets para OBS.
- **PelotenseAtualizarFontes** — com um clique, substitui a URL de todas as fontes de navegador (ex: `https://pelotense-assets.netlify.app/...` → `http://192.168.8.231:5174/...`).

## Como instalar

Baixe o arquivo `PelotenseAssets.dll` na [última release](https://github.com/CristianAHDS/pelotense-obs-plugins/releases) e coloque na pasta de plugins do OBS:

- Windows: `%APPDATA%\obs-studio\plugin-data\` ou dentro da pasta `obs-plugins` da instalação do OBS.

## Desenvolvimento

Este projeto usa o template `obs-plugintemplate/` (CMake) e requer o código-fonte do OBS Studio para compilar.

## Como compilar (Windows)

Para pedir que eu compile este plugin novamente, informe que deseja gerar o build de `PelotenseAssets.dll` no branch `master`.

### Pré-requisitos
- Visual Studio 2022 com workload "Desenvolvimento para desktop em C++"
- CMake (3.21+)
- Git
- Código-fonte do OBS Studio (branche compatível com o template)

### Passos de build
1. Clone este repositório e o repositório do OBS Studio.
2. No diretório `obs-plugintemplate`, gere os arquivos do CMake apontando para o OBS Studio:

```bash
cmake -S . -B build_x64 -A x64 \
  -DOBS_SOURCE_PATH="<caminho-para-o-obs-studio>" \
  -DCMAKE_BUILD_TYPE=Release
```

3. Compile:

```bash
cmake --build build_x64 --config Release
```

4. O plugin será gerado em:

```
obs-plugintemplate/build_x64/Release/PelotenseAssets.dll
```

5. Copie `PelotenseAssets.dll` para a pasta de plugins do OBS:

- `%APPDATA%\obs-studio\plugin-data\`
- ou dentro da pasta `obs-plugins` da instalação do OBS.

### Gerar uma nova release
Após o build, envie o `.dll` como anexo de uma release no GitHub:

```bash
gh release create v1.0.1 --title "v1.0.1" --notes "..." obs-plugintemplate/build_x64/Release/PelotenseAssets.dll
```
