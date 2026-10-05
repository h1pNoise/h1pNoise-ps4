# Teste de atualização com o payload do GoldHEN

Este canal é separado das atualizações normais. Não foi confirmado numa PS4 real.

## Como experimentar 0.1.40 → 0.1.41

1. Instala manualmente `h1pNoise-0.1.40-payload-test.pkg` pelo Package Installer do GoldHEN, com **Enable Background Installation desligada**. Não desinstales a h1pNoise.
2. Mantém **Enable BinLoader Server ligado**, na porta 9090. Abre a h1pNoise 0.1.40 e liga o telemóvel pelo QR.
3. Em **Atualizações da app**, procura a atualização. Deve aparecer 0.1.41. Escolhe **Descarregar atualização**.
4. Depois de descarregar e verificar o PKG, a app entrega o instalador incorporado ao payload server local. Pode demorar até 60 segundos a confirmar a entrega.
5. Só depois dessa confirmação, a h1pNoise fecha. O payload aguarda o processo terminar, prepara uma cópia do PKG no disco e tenta instalar a nova versão. Quando confirma os dados da instalação, tenta abrir a h1pNoise novamente.

Não precisas de instalar a app Updater separada nem de enviar um ficheiro `.bin` manualmente. O instalador já está incorporado nestes PKG. Mantém a consola ligada durante o teste.

## Se não concluir

O original descarregado permanece em `/data/pkg/h1pNoise-update-51.pkg`. O registo é `/data/pkg/update-install-debug.log`. Envia esse registo antes de repetir uma entrega que tenha sido aceite pelo payload.

Se o payload server não estiver disponível ou não confirmar a entrega, a app mantém-se aberta. Se a instalação não for confirmada, o payload não tenta reabrir a app. Se a instalação for confirmada mas a abertura automática falhar, tenta abrir pelo menu e comunica a mensagem exata.

Este teste usa uma instalação nativa depois de fechar a app. Não desinstala o título nem chama PrepareOverwrite. O acesso ao disco e a autorização de instalação são temporários e são restaurados antes de o payload retornar ao GoldHEN. A confirmação verifica o PKG instalado; não garante que o sistema consiga executá-lo. A correção do CE-30012-6 ainda precisa de confirmação na consola.

## Validação

Compilação dos dois PKG e do payload, sem importações ou relocações pendentes. No PC: 22 verificações de entrega, 20 do instalador, 7 de duração/restauração das permissões e testes de regressão do canal normal. As chamadas da PS4 são simuladas nos testes. Não substituem a execução no hardware.

0.1.40: build 50, APP_VER 00.50. 0.1.41: build 51, APP_VER 00.51. TITLE_ID HBRW00001 e CONTENT_ID IV0000-HBRW00001_00-HARBORPS40000000 mantidos. Os canais normais e os canais dos testes anteriores não são alterados por esta publicação.
