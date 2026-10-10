# Real-Debrid — h1pNoise 1.0.1

Build 77: definições numa aba Real-Debrid e seleção apenas dos PKG diretos em magnets com ficheiros extra.

Disponível na release 1.0.1. A instalação das atualizações continua a ser manual pelo GoldHEN.

1. Instala este PKG pelo GoldHEN com **Enable Background Installation** desligado, confirmando a substituição da versão anterior.
2. Abre a h1pNoise e liga o site à consola pelo QR ou pelos quatro números.
3. Abre a aba **Real-Debrid**, ativa o serviço, cola a tua API e escolhe **Guardar API**. É necessária uma conta Premium ativa. Introduz a API apenas no campo da app, nunca no chat.
4. Fecha e volta a abrir a h1pNoise: deve aparecer **Ativo e guardado na consola**, sem voltar a pedir a API. O campo da API permanece vazio; a app nunca devolve a chave ao navegador.
5. Envia primeiro um `.torrent` com vários PKG e escolhe **Só descarregar**: deve preparar e descarregar cada PKG separadamente, incluindo os de subpastas. O Real-Debrid pode criar uma tarefa por PKG; essas tarefas permanecem na tua conta.
6. Envia um magnet v1 de um PKG pequeno que possas usar para testar. Testa também um magnet que inclua imagens ou NFO junto aos PKG: deve apresentar apenas os PKG na lista de transferência. Com Real-Debrid ativo, vai diretamente ao serviço, sem procurar peers e sem precisar de trackers no magnet.
7. Quando os ficheiros aparecerem, escolhe **Descarregar e instalar · Real-Debrid**. Deve preparar nos servidores, guardar o download na consola, verificar os PKG e instalar. Confirma que o título abre.
8. Testa também **Só descarregar**, envio de `.torrent`, pausa/retoma, API inválida e os torrents normais com Real-Debrid desligado.
9. Desativa o Real-Debrid, fecha e abre a app: deve lembrar que está desligado e conservar a API. Volta a ativar sem a colar novamente.
10. Usa **Esquecer API**, fecha e abre a app: deve ficar desligado e pedir uma API nova.

A API e a preferência ficam guardadas num ficheiro privado da app em `/data/pkg/.h1pNoise-real-debrid.conf`. Não é cifrado; quem tiver acesso aos ficheiros da consola pode ler a API. Na PS4 é criado com permissões restritas. O envio do navegador à consola usa a rede local; os pedidos à API e os downloads do Real-Debrid usam HTTPS. A API não é incluída nos registos, na resposta de estado nem no armazenamento do navegador.

As tarefas criadas no Real-Debrid mantêm-se na conta. A pausa interrompe a operação local; a retoma reutiliza a mesma tarefa durante a sessão. Depois de fechar a app, envia novamente o magnet para iniciar uma nova sessão; guardar a API não guarda a transferência em curso. A app não apaga outras tarefas da conta.

Aceita de 1 a 32 PKG diretos com caminhos distintos. Nos magnets, ignora os restantes ficheiros (por exemplo NFO e imagens); não descarrega nem descompacta ZIP/RAR ou partes. O limite de 32 aplica-se aos PKG, não aos ficheiros extra. ZIP/RAR e partes ainda não são suportados. Com `.torrent`, verifica os blocos SHA-1 originais. Com magnet direto, a API não fornece esses blocos: verifica a correspondência do hash do magnet, os nomes e tamanhos recebidos, o tamanho efetivo e o cabeçalho de cada PKG. Isso não equivale à verificação SHA-1 de todos os blocos. A app e a consola precisam de permanecer ligadas.

## Verificação local

84 cenários sintéticos executam o cliente HTTPS da PS4, o envio direto de magnets, a seleção de ficheiros, a escrita, a verificação, a instalação e a persistência. Foram verificadas também as funcionalidades anteriores da interface web, o PKG e a compilação para PS4. Os testes não contactam uma conta real nem torrents públicos. Os testes com uma conta real e a instalação física desta revisão ainda precisam de confirmação na PS4.

Se falhar o download ou a consola continuar a fechar com um magnet, envia `/data/pkg/real-debrid-debug.log`. O registo contém etapas, contagens e códigos HTTP, sem API, links privados ou conteúdo das respostas. A redução do uso de pilha é uma correção de um risco identificado; ainda não confirma a causa do CE-34878-0 na consola.
