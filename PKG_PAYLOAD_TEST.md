# Teste 0.1.46 → 0.1.47: acesso aos ficheiros do instalador

O registo da PS4 confirma agora o arranque do payload, a preparação das permissões e o carregamento dos módulos. A falha acontece dentro do instalador, antes de confirmar a entrega. A causa exata ainda não foi demonstrada: o instalador forçava /user/data, embora o diagnóstico conseguisse escrever por /data.

Este teste procura o pedido primeiro em /data e depois em /user/data. Escolhe um só caminho para o pedido, manifesto, confirmação e PKG de instalação. Acrescenta os passos e o erro interno ao registo por chamadas diretas ao sistema, mesmo se o registo normal não abrir.

1. Fecha a h1pNoise e instala manualmente **h1pNoise-0.1.46-payload-test.pkg**, com **Enable Background Installation desligada** no GoldHEN. Instala por cima; não desinstales primeiro.
2. Mantém **Enable BinLoader Server ligado**. Abre a 0.1.46 e liga o telemóvel pelo QR.
3. Carrega **Procurar atualização**: deve aparecer **0.1.47**. Descarrega a atualização e aguarda até 60 segundos pela confirmação do payload.
4. Se houver erro, a app permanece aberta e o original fica em **/data/pkg/h1pNoise-update-57.pkg**. Envia o novo **/data/pkg/update-install-debug.log**. Não apagues pedidos pendentes para repetir o teste.

O fluxo só fecha o processo da h1pNoise após confirmação independente com o código aleatório da entrega. Depois tenta instalar uma cópia verificada e reabrir a app. Não desinstala a h1pNoise, não precisa do Updater separado e não elimina o PKG original.

Canal e pasta novos (**pkg-payload-v4-test**) isolam o teste dos pedidos pendentes da tentativa anterior. As versões antigas não recebem este canal; é necessário instalar a 0.1.46 manualmente. Os canais dos amigos permanecem inalterados.

Validação no PC: 9 verificações do formato nativo, 34 da entrega com o BIN real, 22 do instalador, 12 das permissões/caminhos e 6 do diagnóstico inicial; 89 verificações do atualizador normal também passaram. As APIs da PS4 são simuladas nestes testes. O arranque e módulos foram confirmados pelo registo real anterior; esta correção da instalação/reabertura ainda precisa de teste na consola.

APP_VER: 00.56 → 00.57 · TITLE_ID: HBRW00001.
