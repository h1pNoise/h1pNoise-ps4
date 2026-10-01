# h1pNoise 0.1.26 — substituição da versão instalada

Trata o erro BGFT 0x80990088 («aplicação já instalada») ao confirmar uma
atualização da h1pNoise. Depois dessa recusa, consulta o slot da app instalada,
prepara a substituição com AppInstUtil e tenta registar uma única vez.
Esta preparação é exclusiva do atualizador, após validar a assinatura,
SHA-512, versão, identidade e tipo do PKG. Não desinstala a aplicação.

Falhas na preparação deixam o PKG guardado para instalação manual. Pedidos
duplicados não são repetidos. Se o registo funcionar mas o arranque ou a
restauração de permissões falhar, preserva o número do pedido e impede outro
registo. Cada chamada nativa tem uma entrada antes e depois em
`/data/pkg/link-debug.log`.

Instala esta correção manualmente uma vez: fecha a h1pNoise e instala o PKG
0.1.26 por cima da versão anterior. O atualizador antigo ainda pode recusar
a substituição com 0x80990088. Não precisas de desinstalar a versão anterior.

Mantém o espaço livre do disco interno, downloads em `/data/pkg`, código
de quatro números e a correção HTTPS da 0.1.25. Disponibiliza o PKG também
pelo endereço raw para compatibilidade com clientes antigos.

Verificação: compilação PS4 OpenOrbis; 165 casos do atualizador, 53 de
validação de PKG/URLs, 44 de envio BGFT, 18 de permissões, 23 do acesso
libjbc e verificações da interface. Executam o código de produção com APIs
PS4 simuladas, incluindo 0x80990088, slot não zero, falhas de preparação,
uma única repetição e recuperação após falha de arranque/restauração.

A substituição na consola real ainda precisa de confirmação. Esta versão
não comprova a resolução do crash CE-34878-0 anterior sem o registo dessa
falha. APP_VER 00.36, TITLE_ID HBRW00001. Experimental.
