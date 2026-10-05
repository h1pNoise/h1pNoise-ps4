# h1pNoise 0.1.55 - acesso direto à raiz USB

O registo real da PS4 13.50 confirmou que a pasta USB abre, mas a criação relativa ao diretório devolve erro 22 e a remoção erro 14 depois da tentativa inicial recusada. A 0.1.52/0.1.53 não chegava à alternativa por caminho absoluto nestes erros.

Esta versão usa diretamente as operações libc/libkernel no caminho fixo da raiz da pen. Obtém e restaura as credenciais em cada abertura, remoção e renomeação; o download e a leitura/escrita do conteúdo decorrem depois da restauração. O ficheiro é verificado na própria pen antes de receber o nome final. Não usa openat/renameat/unlinkat nem copia o PKG para o disco interno. Uma falha mostra o erro e fica registada em /data/pkg/usb-debug.log.

## Testar 0.1.54 -> 0.1.55

1. Fecha a app e instala h1pNoise-0.1.54-experimental.pkg manualmente pelo Package Installer do GoldHEN, com Enable Background Installation desligado.
2. Abre a 0.1.54, atualiza a página no telemóvel e procura a atualização 0.1.55.
3. Liga a pen exFAT/FAT32 diretamente à PS4. Escolhe Descarregar atualização e Pen USB 1 (ou Pen USB 2, conforme o ponto de montagem).
4. Aguarda a confirmação de download verificado. O PKG fica diretamente na raiz da pen: h1pNoise-update-65.pkg.
5. Fecha a app e instala o ficheiro manualmente pelo GoldHEN.

Ambos os PKG desta publicação contêm a mesma correção. É necessário instalar primeiro a 0.1.54 para testar o download USB com o código corrigido. O destino interno /data/pkg continua disponível.

## Validação

187 verificações do atualizador por pacote e testes da interface no PC. Inclui abertura, leitura, escrita e renomeação por caminho absoluto, credenciais restauradas antes da rede e fdopen, falhas de ativação/restauração, pen ausente/removida, erros 1/2/13/14/22/30/78, falha fdopen, falta de espaço e preservação das cópias anteriores. Os testes recusam operações relativas ao diretório. As chamadas PS4 são simuladas no PC; a escrita numa pen na PS4 real ainda requer confirmação.
