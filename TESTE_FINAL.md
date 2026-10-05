# Teste final — 0.1.56 → 0.1.58

A 0.1.58 é a candidata de teste antes da h1pNoise 1.0.0. Mantém as funções da 0.1.56 e atualiza os textos e instruções. Faz os testes por esta ordem e indica **OK** ou a mensagem exata da falha. Se estes pontos já foram confirmados na 0.1.56, basta repetir na candidata para verificar que continuam a funcionar.

## 1. Atualizar para a candidata

1. Abre a **0.1.56** e liga o telemóvel pelo QR.
2. Em Atualizações da app, carrega **Procurar atualização**: deve aparecer **0.1.58**.
3. Descarrega para **disco interno**. Confirma que termina e mostra `/data/pkg/h1pNoise-update-68.pkg` como verificado.
4. Usa **Guardar outra cópia** e escolhe a **pen USB**. Confirma que o mesmo PKG aparece na raiz da pen.
5. Fecha a app e instala um destes PKG pelo Package Installer do GoldHEN, com **Enable Background Installation desligado**. Aceita substituir; não desinstales primeiro.
6. Abre a app e atualiza a página do telemóvel. Deve aparecer **0.1.58** na televisão e nas informações da página. Procurar atualização deve indicar que está atualizada.

## 2. Ligação, cor e disco

- Liga pelo **QR** e depois pelo **código de quatro números**.
- No topo da página, escolhe uma cor no botão Cor. Confirma que muda os detalhes e mantém a escolha ao recarregar. Testa Cor personalizada e Voltar ao verde.
- Confirma que a medição de espaço livre é plausível em relação às definições de armazenamento da PS4.

## 3. Transferências

Usa conteúdo PKG que tens autorização para descarregar, com uma fonte que sabes estar disponível.

- **Torrent:** inicia um download, pausa, retoma e confirma a conclusão. Se usares Descarregar e instalar, confirma também o resultado na consola.
- **Magnet:** testa uma ligação com tracker HTTP/UDP ou fonte direta suportada e confirma que obtém os dados e inicia o download.
- **Link PKG:** envia um link direto válido; confirma que o pedido aparece nas Transferências da PS4, descarrega e instala. Se ficar sem imagem/A calcular, distingue isso de uma transferência que não avança e indica se o conteúdo abre após instalar.

Mantém a PS4 ligada e a app aberta para torrents/magnets. Não precisas de testar repouso para esta publicação.

## 4. Limpeza e reabertura

- Guarda um PKG que não seja uma atualização da h1pNoise no destino escolhido.
- Carrega **Apagar updates antigas**, escolhe o destino e confirma. Verifica que só desapareceram h1pNoise-update de builds anteriores a 68; o outro PKG e o h1pNoise-update-68.pkg ficam guardados.
- Fecha e volta a abrir a app: confirma o arranque, a versão e a ligação pelo telemóvel.

## Resposta

Indica o firmware e a versão GoldHEN, e o resultado de: **atualização interna / USB / instalação por cima / QR e código / espaço livre / torrent / magnet / link PKG / limpeza / reabertura**. Nos pontos já testados, basta OK. Qualquer erro: copia a mensagem e diz em que passo apareceu.

Depois desta confirmação, publicamos a **1.0.0** como Latest, com o canal assinado atualizado e as instruções definitivas. Compatibilidade noutros firmwares só é anunciada depois de testes próprios.
