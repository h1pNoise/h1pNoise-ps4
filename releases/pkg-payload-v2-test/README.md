# Correção do formato do payload: teste 0.1.42 → 0.1.43

O teste 0.1.40 → 0.1.41 foi recusado pelo GoldHEN com **Invalid payload format**, antes de confirmar a entrega ou iniciar a instalação. O BIN anterior começava diretamente pelo código de preparação da memória. O novo começa com **JMP rel32 (E9)**, como os payloads BIN convencionais. A entrada e todos os endereços são produzidos pelo linker, sem acrescentar bytes posteriormente ao ficheiro.

## Como experimentar

1. Instala manualmente **h1pNoise-0.1.42-payload-test.pkg** pelo Package Installer do GoldHEN, com **Enable Background Installation desligada**, por cima da versão atual.
2. Mantém **Enable BinLoader Server ligado**, na porta 9090. Abre a h1pNoise 0.1.42 e liga o telemóvel pelo QR.
3. Em **Atualizações da app**, procura a **0.1.43** e escolhe **Descarregar atualização**.
4. Após descarregar e verificar o PKG, a app envia o payload incorporado ao GoldHEN. A confirmação pode demorar até 60 segundos. Só depois dessa confirmação a h1pNoise fecha.
5. O payload espera que o processo termine, prepara uma cópia no disco e tenta instalar. Após confirmar o PKG instalado, tenta abrir a nova versão.

Não precisas de instalar o Updater separado nem de enviar um BIN manualmente. Mantém a consola ligada durante este teste.

O canal e a pasta dos pedidos deste teste são novos: não reutilizam o pedido deixado pelo payload recusado da 0.1.40. Não apagues pedidos anteriores para tentar desbloquear esta versão. As versões 0.1.40/0.1.41 não recebem a 0.1.43 no canal antigo: precisam da instalação manual da 0.1.42 para usar o payload corrigido.

## Se falhar

Envia **/data/pkg/update-install-debug.log**, incluindo qualquer mensagem ao abrir. O original continua em **/data/pkg/h1pNoise-update-53.pkg**, para recuperação manual. Se o payload não confirmar a entrega, a app mantém-se aberta. Uma entrega aceite sem confirmação da instalação bloqueia novas tentativas até o registo ser analisado.

O formato foi corrigido e verificado no PC. **A aceitação pelo GoldHEN e a instalação/abertura na PS4 real ainda estão por confirmar.** A confirmação do PKG instalado não prova que o sistema consegue executá-lo; o CE-30012-6 precisa de confirmação no hardware.

## Validação

Compilação dos dois PKG e do payload, com E9 no primeiro byte e salto para o arranque na posição 16. Sem importações ou relocações pendentes; a área BSS cabe no BIN enviado. Nove verificações do formato, incluindo o BIN antigo rejeitado, 30 da entrega usando o BIN realmente compilado, 20 do instalador, sete da restauração das permissões e 89 do canal normal. As chamadas da PS4 são simuladas.

Não desinstala o título nem chama PrepareOverwrite. Conserva o PKG original, espera pelo fecho do processo e restaura as credenciais do anfitrião quando o payload termina. Os canais normais e anteriores ao teste de payload mantêm-se inalterados.

0.1.42: build 52, APP_VER 00.52. 0.1.43: build 53, APP_VER 00.53. TITLE_ID HBRW00001 e CONTENT_ID IV0000-HBRW00001_00-HARBORPS40000000 mantidos.

Referência da entrada BIN: [DirectPackageInstaller crt.asm](https://github.com/marcussacana/DirectPackageInstaller/blob/ca7bade66737fede3b0e7bad73b665e0a4d4ff39/Payload/lib/crt.asm).
