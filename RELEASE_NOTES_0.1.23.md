# h1pNoise 0.1.23 — medição do espaço em /data/pkg

Acrescenta uma consulta direta ao syscall fstatfs moderno da PS4 (397),
antes do método anterior da biblioteca OpenOrbis. Esta tentativa evita
depender exclusivamente do export _fstatfs usado pela biblioteca.
A causa da falha na PS4 do utilizador ainda não está confirmada.

Os dados são validados com o prefixo de 64 bits da estrutura FreeBSD 9 / PS4:
versão, dimensão do bloco, total de blocos e blocos livres/disponíveis.
Uma resposta vazia, incoerente ou com overflow continua desconhecida, e
não é apresentada como zero ou como uma capacidade estimada.
O espaço genuinamente esgotado continua a ser reportado e bloqueia novas
operações que exijam espaço. Uma resposta desconhecida continua sem bloquear
os downloads, e os erros reais de escrita permanecem ativos.

O fallback para /data só é usado quando stat confirma que /data/pkg está
no mesmo dispositivo. Não se consulta outro disco para preencher o valor.
Não há alteração de credenciais, roots, memória do kernel ou destino.
Downloads continuam em /data/pkg e o envio de links mantém a implementação
0.1.22, cujo download o utilizador confirmou na consola.

Referências consultadas:
https://github.com/OpenOrbis/musl/blob/master/src/stat/statvfs.c
https://github.com/ps4dev/ps4sdk/blob/master/include/sys/mount.h
https://github.com/freebsd/freebsd-src/blob/releng/9.0/sys/sys/syscall.h

Um registo por execução é escrito em /data/pkg/storage-debug.log, com o
resultado da consulta nativa, da biblioteca e os campos devolvidos. Não
contém links, tokens ou o código de emparelhamento. Se a medição continuar
indisponível, copiar este ficheiro permite identificar a falha na consola.

Verificação: compilação OpenOrbis; 17 casos da consulta PS4, 16 do emulador
e testes das políticas de armazenamento em Windows/PS4/shadPS4. Cobrem
valores acima de 4 GB, disco cheio, disponibilidade negativa, respostas
inválidas, overflow, erros, fecho de descritores e proteção do mesmo volume.
Inspeção do objeto x86-64 confirma syscall 397 e tratamento do carry/erro.
Os testes simulam o sistema de ficheiros; requer confirmação na PS4 real.

APP_VER 00.33, TITLE_ID HBRW00001. Instalar por cima da 0.1.22 depois de
terminarem os downloads/instalações em curso. Versão experimental local.
