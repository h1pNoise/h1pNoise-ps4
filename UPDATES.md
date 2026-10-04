# Atualizações da h1pNoise

## Magnets — 0.1.29

A release 0.1.29 acrescenta magnets e é distribuída para instalação manual,
com a app fechada. O feed antigo continua na 0.1.25: não anuncia esta release
às versões com o instalador problemático. As versões posteriores não devem
ser anunciadas nesse feed antes de confirmar um caminho de instalação
que funcione numa PS4 real. Não uses Instalar agora nas versões 0.1.26/0.1.27.

O código contém um protótipo opcional de atualizações do executável
(--runtime), com canal separado e confirmação de arranque. Não está ativo
nos PKG da 0.1.29 e ainda não foi confirmado numa PS4 real.

## Recuperação 0.1.28

Não uses **Instalar agora** nas versões 0.1.26/0.1.27. No teste numa PS4
real, a app desapareceu sem criar um pedido nas Transferências. A 0.1.28
retira a preparação da substituição e suspende a instalação pela própria
app. A procura e o download verificado continuam disponíveis; a instalação
do PKG deve ser feita manualmente com a h1pNoise fechada.

Se a app desapareceu durante o teste para a 0.1.27, o ficheiro
`/data/pkg/h1pNoise-update-37.pkg` pode reinstalá-la manualmente. Instala
depois a recuperação 0.1.28, disponível no GitHub Releases. A app guarda
os PKG e mantém os torrents, o espaço livre e o destino `/data/pkg`.

O feed foi reposto na 0.1.25 para retirar o aviso problemático às versões
0.1.26/0.1.27. Não anunciamos a recuperação 0.1.28 nesse feed: as apps
antigas ainda executariam o instalador antigo. Uma instalação automática
precisa de um instalador independente da aplicação que está a ser substituída,
com confirmação numa PS4 real. Não foi implementado nesta recuperação.

A página 0.1.28 deixa de mostrar a confirmação de instalação. O servidor
recusa o pedido de instalação de uma página antiga e preserva o PKG verificado.
As notas completas estão em [RELEASE_NOTES_0.1.28.md](RELEASE_NOTES_0.1.28.md).

## Manifestos e publicação

Repositório: https://github.com/h1pNoise/h1pNoise-ps4

A app consulta `releases/current/update.h1p` no endereço HTTPS raw do
repositório. Não usa a marca Latest do GitHub para escolher a versão.
Cada manifesto tem 64 bytes de assinatura Ed25519 e nove linhas UTF-8:
magic, CONTENT_ID, versão, APP_VER SFO, build, tamanho, SHA-512, URL HTTPS
do PKG e notas. A chave original mantém-se; a chave privada nunca é publicada.
O manifesto tem no máximo 4096 bytes e o PKG 128 MiB.

Para preparar uma versão, aumenta APP_VERSION, APP_BUILD e APP_SFO_VERSION
sem alterar TITLE_ID/CONTENT_ID; compila e valida o pacote PS4 normal;
cria uma tag nova e disponibiliza o PKG com as notas. Nunca substituas uma
tag publicada. Só anuncia um manifesto no feed após verificar o download
público, a assinatura e a compatibilidade das versões antigas.

Nesta fase, as versões de recuperação são distribuídas para instalação
manual, sem atualizar o feed da app. Mantém os certificados HTTPS ativos,
a validação da assinatura, SHA-512, identidade e versão do PKG.
