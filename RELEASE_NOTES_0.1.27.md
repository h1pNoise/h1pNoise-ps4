# h1pNoise 0.1.27 — teste do atualizador a partir da 0.1.26

Esta versão mantém as funcionalidades e o instalador da 0.1.26. Aumenta
apenas a versão e o número da build para permitir um teste real de procura,
download, verificação e substituição da app através do atualizador.

Para testar numa PS4 real com a 0.1.26 instalada:

1. Abre a h1pNoise, liga o telemóvel pelo QR e entra em Atualizações da app.
2. Carrega em Procurar atualizações: deve indicar a 0.1.27 disponível.
3. Carrega em Descarregar atualização e espera pela confirmação de verificação.
4. Carrega uma vez em Instalar agora. Quando indicar que foi enviada ao
   sistema, fecha a h1pNoise e acompanha Notificações > Transferências.
5. Depois de concluir, abre a app: deve indicar Versão instalada: 0.1.27.
   Uma nova procura deve indicar que tens a versão mais recente publicada.

O teste depende de uma PS4 real. Windows e shadPS4 não instalam atualizações.
O PKG verificado fica em `/data/pkg/h1pNoise-update-37.pkg`. Se a instalação
falhar, guarda a mensagem exata e `/data/pkg/link-debug.log`; não repitas o
pedido enquanto houver uma tarefa nas Transferências. O PKG da release
também permite instalar manualmente com a app fechada.

Experimental: a substituição da app no hardware real ainda precisa de
confirmação. Mantém a preparação AppInstUtil após 0x80990088, uma única
nova tentativa, prevenção de duplicados e registo das chamadas nativas.
Não desinstala a app. Mantém a chave de assinatura original e a validação
de SHA-512, identidade e versão do pacote.

Validação: compilação PS4, 165 casos do atualizador com APIs PS4 simuladas
e verificação do PKG e do manifesto assinado. APP_VER 00.37,
TITLE_ID HBRW00001, build 37. O espaço interno e o destino `/data/pkg`
mantêm-se iguais à 0.1.26.
