# Atualizações da h1pNoise

## Guardar a atualização — 0.1.48

A versão normal 0.1.48 volta ao canal `manual-v1` e acrescenta a escolha de
destino antes do download: interno (`/data/pkg`) ou raiz da pen USB detetada
(`/mnt/usb0` ou `/mnt/usb1`). Não cria pontos de montagem USB nem muda o destino
dos torrents. Se a pen desaparecer ou a escrita falhar, o ficheiro incompleto
não é anunciado como pronto. A assinatura, hash e identidade são verificados
antes de disponibilizar o PKG para a instalação manual pelo GoldHEN.
A app não fecha nem inicia instalação após o download. **Guardar outra cópia**
permite repetir o download para outro destino, preservando a primeira cópia.

Instala a 0.1.48 manualmente uma vez para sair dos canais de teste do payload.
Para testar a escolha de destino na consola, será necessária uma atualização
posterior anunciada no canal manual. A disponibilidade USB é consultada novamente
antes de escrever, mesmo que a pen estivesse ligada ao abrir a página.

## Teste real 0.1.30 → 0.1.31

O utilizador confirmou que a instalação por cima funciona com **Enable
Background Installation desligado**. Com essa opção ligada, recebia
`Package already installed`. Fecha a h1pNoise, desliga a opção no GoldHEN,
instala o PKG pelo Package Installer e aceita substituir/reinstalar se
for pedido. Não desinstales primeiro.

Para testar o canal: mantém a 0.1.30 instalada, procura uma atualização e
confirma que aparece 0.1.31 / **Nova versão**. Descarrega pela página:
`/data/pkg/h1pNoise-update-41.pkg`. Instala pelo procedimento acima,
reabre a h1pNoise e confirma **Versão instalada: 0.1.31**. Uma nova procura
deve mostrar **Atualizada** enquanto o canal anunciar a build 41.
O aviso e o download real na consola ainda precisam de confirmação.

## Canal separado — a partir da 0.1.30

Instala a 0.1.30 manualmente uma vez para aderir ao novo canal. Os PKG desta
versão usam `releases/manual-v1/current.h1p` no ramo `main`, com a mesma
chave pública Ed25519. Ao abrir a app ou carregar **Procurar atualização**,
ela compara a build instalada com a anunciada. Uma build superior mostra
o aviso na consola e na página e permite descarregar o PKG verificado para
`/data/pkg/h1pNoise-update-<build>.pkg`. Fecha a app e instala pelo GoldHEN.
Não há instalação automática, remoção da app, nem substituição BGFT.

O canal legado `releases/current/update.h1p` permanece na 0.1.25. As versões
0.1.26/0.1.27 não conhecem o endereço novo e não recebem os avisos dele.
O canal experimental de executáveis `releases/runtime/current.h1p` também
é separado e não está ativo nos PKG normais.

Para publicar uma futura atualização no canal manual:

1. Aumenta versão, build e APP_VER, mantendo TITLE_ID e CONTENT_ID.
2. Compila e valida o PKG para a PS4 real; publica uma tag e asset novos.
3. Descarrega o asset público e confirma que é idêntico ao PKG validado.
4. Usa `tools/release_update.py --channel manual-v1`, com o PKG real, a
   versão, build, URL HTTPS do asset, notas e chave privada fora do projeto,
   para gerar `releases/manual-v1/current.h1p`. A chave privada nunca vai
   para o GitHub. O assinador recusa builds anteriores à 40 e o PKG shadPS4.
5. Verifica assinatura, hash, tamanho e identidade do manifesto e publica
   apenas esse ficheiro assinado no ramo `main`. Confirma o download público
   e a simulação do aviso/download antes de anunciar aos utilizadores.

Publicar apenas a release, ou mudar **Pre-release/Latest**, não anuncia
uma versão neste canal. O manifesto inicial anuncia 0.1.30 / build 40:
a própria 0.1.30 mostra **Atualizada**; uma versão futura anunciada com
build superior apresenta **Nova versão**. Se o canal ficar para trás,
mostra **Canal desatualizado**. Erros de rede ou de assinatura nunca são
tratados como confirmação de que a app está atualizada.

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

A app até à 0.1.29 consulta `releases/current/update.h1p`; a partir da
0.1.30 consulta `releases/manual-v1/current.h1p` no endereço HTTPS raw do
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

As versões de recuperação anteriores à 0.1.30 são distribuídas para
instalação manual, sem atualizar o canal legado. As novas versões podem
ser anunciadas apenas no canal separado `manual-v1`. Mantém os certificados
HTTPS ativos, a validação da assinatura, SHA-512, identidade e versão do PKG.

## USB a partir de 0.1.50

A partir de 0.1.56, **Apagar updates antigas** pede o destino e confirmação. Apaga só os nomes fixos `h1pNoise-update-<build>.pkg` e `.pkg.part` de builds anteriores a APP_BUILD, diretamente no destino escolhido. Preserva a instalada e as versões mais recentes, outros PKG, logs e subpastas. Não faz limpeza recursiva nem desinstala aplicações. Requer autenticação da consola e não pode decorrer em simultâneo com transferências ou atualizações. O resultado da limpeza é separado do estado da atualização pronta para instalar.

A selecao USB permanece disponivel mesmo quando a pasta nao e visivel na raiz isolada da app. O acesso e verificado antes do download. A pasta real e aberta com acesso temporario quando necessario; as credenciais sao restauradas antes de iniciar a rede. As operacoes no PKG ficam ancoradas ao diretorio aberto, com nomes fixos e sem criar pastas USB. O registo de falha de acesso e `/data/pkg/usb-debug.log`. A instalacao continua manual pelo GoldHEN.

A partir de 0.1.54, as operações relativas ao diretório são substituídas pelo caminho absoluto fixo na raiz USB. O registo real da PS4 13.50 mostrou erros 22/14 nas operações relativas, mesmo depois de a pasta abrir. Cada abertura/remoção/renomeação usa permissões temporárias e restaura-as antes de iniciar a rede ou usar o ficheiro. A 0.1.55 inclui o PKG 0.1.54 para testar imediatamente este download na pen.

## Escrita USB a partir de 0.1.52

Se as operações no diretório externo falharem por permissões ou não forem suportadas pelo firmware, a app tenta permissões temporárias e depois o caminho absoluto fixo na raiz USB. As credenciais são restauradas antes de usar o ficheiro e iniciar a rede. O PKG continua diretamente na raiz da pen, sem subpasta ou cópia interna. O registo inclui a versão, o passo e os códigos das falhas. A release 0.1.53 inclui o PKG 0.1.52 para testar imediatamente esta correção no download da 0.1.53. Não anuncia alterações nos canais dos instaladores antigos.
