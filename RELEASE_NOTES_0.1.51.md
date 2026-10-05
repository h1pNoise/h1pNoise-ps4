# h1pNoise 0.1.51 - acesso USB corrigido

A escolha USB deixa de ficar bloqueada quando a pasta da pen nao e visivel a partir da app. A verificacao usa uma abertura de diretorio, sem depender do layout stat do SDK. Ao selecionar USB, se necessario a app abre a pasta real da pen com acesso temporario do GoldHEN e restaura as credenciais antes do download. A escrita, verificacao SHA-512/identidade/versao e renomeacao usam o mesmo diretorio aberto. O PKG vai diretamente para a pen; nao ha copia temporaria no disco interno.

## Testar 0.1.50 -> 0.1.51

1. Fecha a h1pNoise e instala h1pNoise-0.1.50-experimental.pkg pelo Package Installer do GoldHEN, com Enable Background Installation desligado.
2. Abre a 0.1.50 e atualiza a pagina do telemovel. Em Atualizacoes da app, procura a 0.1.51.
3. Liga a pen exFAT/FAT32 diretamente a PS4. Carrega Descarregar atualizacao e escolhe Pen USB 1; se a tua pen estiver no segundo ponto de montagem, escolhe Pen USB 2.
4. O ficheiro verificado fica na raiz da pen: h1pNoise-update-61.pkg. O disco interno continua opcional: /data/pkg/h1pNoise-update-61.pkg.
5. Fecha a app e instala manualmente pelo GoldHEN. A app nao se fecha nem instala automaticamente.

Se nao for possivel abrir a pen, aparece o motivo. Uma falha de acesso temporario fica registada em /data/pkg/usb-debug.log. Nao e criado um ponto de montagem USB e nao se muda silenciosamente para o disco interno.

## Validacao

145 verificacoes do atualizador para cada versao e testes da interface. Inclui pen ausente, pasta USB fora da raiz da app, restauracao das credenciais antes da rede, diretorio e descritores fechados, falta de espaco, falhas de escrita/flush/renomeacao e preservacao dos ficheiros anteriores. As chamadas PS4 sao simuladas no PC; a gravacao numa pen na PS4 real precisa de confirmacao.
