# h1pNoise 0.1.28 — recuperação do atualizador

Um teste numa PS4 real confirmou que, nas versões 0.1.26/0.1.27, confirmar
a instalação podia remover a app atual sem criar um pedido nas Transferências.
A chamada AppPrepareOverwritePkg não é uma preparação reversível da app em
execução; o método introduzido na 0.1.26 não deve ser usado para autoatualização.

Esta versão remove essa chamada e o respetivo mecanismo de repetição. A app
continua a procurar, descarregar e verificar atualizações, mas a instalação
da própria h1pNoise fica suspensa. A página deixa de mostrar Instalar agora.
O servidor também recusa a instalação pela própria app se uma página antiga
tentar pedi-la: o PKG e a instalação existente são preservados.

Instala a 0.1.28 manualmente com a h1pNoise fechada. Se a app desapareceu
durante o teste, o PKG 0.1.27 descarregado continua em
`/data/pkg/h1pNoise-update-37.pkg` e pode reinstalá-la manualmente; depois
instala esta recuperação. Não uses Instalar agora nas versões 0.1.26/0.1.27.

O feed foi reposto na 0.1.25 para retirar o aviso da atualização problemática.
A 0.1.28 não é anunciada nesse feed, porque a 0.1.26/0.1.27 ainda executariam
a operação antiga ao tentar instalá-la. O instalador de jogos/links PKG, os
torrents, a medição do disco e o destino `/data/pkg` mantêm-se.

Validação: compilação PS4, 88 casos do atualizador, 53 de validação de PKG,
44 de envio BGFT, 18 de permissões, 23 de libjbc e testes da interface.
O teste de regressão confirma zero preparação, zero registo/arranque de
autoatualização, preservação do PKG e ausência dos imports nativos de
substituição/remoção no atualizador. A página não envia instalação mesmo
quando recebe o estado de atualização pronta.

Esta recuperação não implementa a instalação automática. Para retomá-la é
necessário um instalador independente da app em execução e teste na consola.
APP_VER 00.38, build 38, TITLE_ID HBRW00001. Experimental.
