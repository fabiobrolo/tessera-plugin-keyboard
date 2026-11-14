# Type Text (Tessera Plugin)

Plugin para **Tessera / ESP Screen Manager** que adiciona um teclado virtual na tela para digitação e envio de texto para scripts do **Home Assistant**.

## Funcionalidades
- **Teclado Virtual na Tela:** Interface amigável com campo de texto, botão de limpar e botão de envio rápido (também acionado pela tecla verde de confirmação do teclado).
- **Integração com Home Assistant:** Chama um script do Home Assistant configurado, passando o texto digitado no parâmetro `input_text`.
- **Card e Tile:** Pode ser adicionado como um card em página ou acionado como ação de toque (*tap action*) a partir de outros cards e entidades.
- **Ação ESPHome:** Fornece a ação `type_text.open` para abrir o teclado via automação local do ESPHome.

## Instalação no Tessera Add-on

1. No Home Assistant, abra o add-on **ESP Screen Manager** (Tessera).
2. Vá até a seção de Plugins / Loja de Plugins.
3. Adicione o repositório do plugin:
   ```text
   https://github.com/fabiobrolo/tessera-plugin-type-text
   ```
4. Adicione o plugin **Type Text** à sua tela.
5. Na configuração do plugin, selecione o **Script de destino** desejado (o script deve aceitar o campo `input_text`).

## Exemplo de Script no Home Assistant

Crie um script no Home Assistant com o campo `input_text`:

```yaml
sequence:
  - action: androidtv.adb_command
    target:
      entity_id: media_player.androidtv_salao_festas_adb
    data:
      command: >-
        am start -a android.intent.action.VIEW -d "vnd.youtube://www.youtube.com/results?search_query={{ input_text | urlencode }}"
fields:
  input_text:
    description: Texto digitado no teclado
    name: Texto
    required: true
    selector:
      text: {}
```

## Licença

MIT License - Veja o arquivo [LICENSE](LICENSE) para mais detalhes.
