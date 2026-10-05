# Atualizações da h1pNoise

## Utilizar na PS4

Em **Atualizações da app**, escolhe **Procurar atualização**. A app também consulta o canal ao abrir. Uma build mais recente mostra a nova versão na televisão e na app ou no site.

Em **Descarregar atualização**, escolhe disco interno (/data/pkg), Pen USB 1 (/mnt/usb0) ou Pen USB 2 (/mnt/usb1). O PKG fica diretamente na raiz da pen exFAT/FAT32, ou no destino interno, com o nome `h1pNoise-update-<build>.pkg`. A app verifica assinatura, hash, identidade e versão. **Guardar outra cópia** permite repetir o download para outro destino.

Fecha a h1pNoise e instala o ficheiro pelo **Package Installer do GoldHEN**, com **Enable Background Installation desligado**. Confirma a substituição se for pedida, sem desinstalar primeiro. Reabre e confirma a versão instalada.

A instalação automática da própria app não faz parte deste canal. O PKG instalado e os ficheiros descarregados permanecem guardados durante o download.

## Limpar ficheiros antigos

**Apagar updates antigas** pede destino e confirmação. Só apaga `h1pNoise-update-<build>.pkg` e `.pkg.part` de builds anteriores a APP_BUILD, diretamente no destino escolhido. Preserva a instalada, as versões mais recentes, os outros PKG, logs e subpastas. Não limpa recursivamente nem desinstala aplicações.

A limpeza requer o código da consola e não decorre em simultâneo com transferências ou atualizações. Mantém o estado de uma atualização já descarregada e mostra o resultado separadamente. Uma falha interrompe a operação e informa quantos ficheiros foram apagados.

## Canal de publicação

Os PKG normais, a partir da 0.1.30, consultam `releases/manual-v1/current.h1p` no ramo main, com a chave pública Ed25519 original. A comparação usa a build e APP_VER. Publicar uma release ou marcar Latest não basta: também é necessário anunciar o manifesto assinado.

O canal legado `releases/current/update.h1p` e os canais de teste dos instaladores/runtime ficam separados. Não os alteres numa publicação normal. Versões antigas que não conhecem manual-v1 precisam de uma instalação manual inicial para aderir ao canal atual.

## Publicar uma atualização

1. Aumenta APP_VERSION, APP_BUILD e APP_SFO_VERSION, mantendo TITLE_ID e CONTENT_ID.
2. Compila e valida o PKG normal para PS4 real; não anuncies um PKG exclusivo do shadPS4.
3. Cria uma tag nova e carrega o PKG e as instruções. Não substituas uma tag publicada.
4. Confirma que o asset público tem o tamanho e hash do PKG validado.
5. Cria o manifesto com `tools/release_update.py --channel manual-v1`, usando a chave privada fora do projeto.
6. Verifica a assinatura com a chave pública, SHA-512, identidade, versão e URL. Só depois publica os bytes exatos em releases/manual-v1/current.h1p.
7. Confirma que os restantes canais e ficheiros das releases foram preservados.

Para uma versão estável, usa uma release sem Pre-release e marcada Latest. A 1.0.0 é a versão final publicada a pedido do utilizador, com APP_VER 01.00 e build 71. A publicação do canal pode demorar alguns minutos a chegar a todos os pontos de acesso do GitHub; repete Procurar atualização depois desse intervalo.

## Formato assinado

64 bytes de assinatura Ed25519, seguidos de nove linhas UTF-8: magic, CONTENT_ID, versão, APP_VER, build, tamanho, SHA-512, URL HTTPS e notas. Máximo de 4096 bytes para o manifesto e 128 MiB para o PKG. Mantém a validação HTTPS e a chave pública original. A chave privada nunca é publicada.

## Diagnóstico USB

As operações usam os caminhos fixos na raiz da pen, com permissões temporárias restauradas antes da rede e do uso do conteúdo. A app não cria pontos de montagem USB nem muda silenciosamente para o disco interno. Se o acesso falhar, a mensagem mostra o motivo; o registo fica em `/data/pkg/usb-debug.log`.
