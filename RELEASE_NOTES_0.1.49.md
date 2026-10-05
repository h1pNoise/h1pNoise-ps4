# h1pNoise 0.1.49 — teste do destino da atualização

Instala a 0.1.48 manualmente e procura esta atualização no canal manual.
Escolhe o disco interno ou a pen USB; o ficheiro verificado é
`h1pNoise-update-59.pkg`. Instala depois manualmente pelo GoldHEN.

A 0.1.49 mantém o mesmo comportamento da 0.1.48, com versão/SFO superior
para permitir testar o download real e a substituição manual.

- Antes de descarregar, escolhe o disco interno ou uma pen USB detetada.
- Interno: `/data/pkg/h1pNoise-update-59.pkg`; USB: raiz de `/mnt/usb0` ou `/mnt/usb1`.
- O PKG continua a ser verificado pela assinatura, SHA-512 e identidade da app.
- Ao terminar, mostra o caminho guardado e permite guardar outra cópia.
- A instalação é manual: fecha a app, desliga Enable Background Installation e instala pelo Package Installer do GoldHEN. Confirma a substituição se for pedida.
- Não precisa do Updater nem do BinLoader. A versão instalada é preservada.

Instala esta versão manualmente uma vez para sair das versões de teste do payload.
A escolha USB será usada nos downloads das próximas atualizações do canal manual.
Usa uma pen exFAT ou FAT32 e mantém-na ligada até terminar. O teste de escrita
numa pen ligada à PS4 real ainda depende de confirmação do utilizador.
