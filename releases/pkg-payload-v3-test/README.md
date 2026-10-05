# Diagnóstico do arranque: teste 0.1.44 → 0.1.45

O registo da tentativa anterior mostra que a app enviou o BIN, mas não confirma o arranque do instalador. Ainda não identifica a causa na PS4. Este teste corrige a falta de diagnóstico nas saídas anteriores ao carregamento da libc e dos módulos; não apresenta a atualização automática como reparada.

1. Instala manualmente **h1pNoise-0.1.44-payload-test.pkg** pelo Package Installer do GoldHEN, com **Enable Background Installation desligada**, por cima da versão atual.
2. Mantém **Enable BinLoader Server ligado** (9090). Abre a app e liga o telemóvel pelo QR.
3. Procura a **0.1.45** e carrega em **Descarregar atualização**. Mantém a consola ligada e aguarda até 60 segundos pela confirmação.
4. Se faltar a confirmação, a app fica aberta, guarda o PKG e apresenta o último passo registado, quando disponível. Envia **/data/pkg/update-install-debug.log** depois desta tentativa.

O log regista versão e tamanho do BIN, entrada no payload, preparação das permissões, módulos e APIs. A escrita inicial usa chamadas diretas ao sistema, sem depender da libc nem de permissões previamente preparadas; tenta o caminho visível e o caminho físico. Falhas de escrita continuam sem bloquear o retorno ao GoldHEN. Se nenhum caminho estiver acessível ou o payload não executar, pode continuar sem registo de arranque.

Os dados de diagnóstico nunca substituem a confirmação com o código aleatório da entrega: mesmo um estado «terminou» não permite fechar a app sem essa confirmação. O pedido usa uma pasta e um canal novos para não repetir o pedido pendente anterior. Não apagues pedidos antigos para desbloquear o teste.

Após confirmação, o fluxo existente tenta fechar a app, preparar a cópia, instalar e reabrir. O PKG original permanece em **/data/pkg/h1pNoise-update-55.pkg**. Não instala o Updater separado, não desinstala a h1pNoise e não requer enviar um BIN manualmente.

Validação no PC: 9 verificações do formato nativo, 34 da entrega com o BIN real, 20 do instalador, 7 das permissões e 6 do diagnóstico inicial. As APIs da PS4 são simuladas; o funcionamento no hardware continua por confirmar.

APP_VER: 00.54 → 00.55 · TITLE_ID: HBRW00001 · canal: pkg-payload-v3-test.
