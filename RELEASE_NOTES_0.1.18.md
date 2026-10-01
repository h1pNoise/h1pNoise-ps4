# h1pNoise 0.1.18 — teste local PS4

Tentativa de corrigir o erro AppInstUtil 0x80020002 quando a app não consegue
abrir o módulo em /system/common/lib, apesar de este aparecer no FTP.

A app verifica se o HEN disponibiliza a chamada necessária à libjbc e se
consegue reconhecer o processo atual. Quando necessário, tenta ativar acesso
temporário aos módulos durante a inicialização de AppInstUtil/BGFT, voltando
depois às credenciais anteriores. Falhas são registadas em link-debug.log.
O descritor do registo permanece aberto durante a mudança de raiz.

Mantém o código de ligação de quatro números e os torrents em /data/pkg.
Esta alteração está limitada à compilação da consola; shadPS4 continua sem
instalação de PKG pelos serviços da PS4.

Verificação: compilação OpenOrbis concluída, 42 testes de validação de PKG,
25 testes da fila PS4, 9 testes do controlo de acesso e testes da página web.
Estes testes simulam os serviços do sistema e a libjbc. Não validam as
alterações de credenciais no kernel, a compatibilidade do HEN com 13.50 ou
a instalação na consola. É uma versão experimental para esse teste.

Esta compilação não foi publicada no GitHub. A origem e as alterações da
biblioteca estão documentadas em src/vendor/libjbc/PROVENANCE.md.
