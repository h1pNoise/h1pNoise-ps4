# h1pNoise — PS4 0.1.27

## Atualização 0.1.27 — teste do atualizador

Mantém as funcionalidades da 0.1.26 e aumenta a versão para permitir testar
uma atualização completa a partir dessa build numa PS4 real. Segue os passos
nas [notas da versão](RELEASE_NOTES_0.1.27.md). A substituição no hardware
real ainda precisa de confirmação.

## Atualização 0.1.26 — substituição da versão instalada

Trata a recusa BGFT 0x80990088 ao atualizar a h1pNoise: consulta o slot
instalado, prepara a substituição e repete o registo uma única vez.
Instala esta correção manualmente, com a app fechada, por cima da versão
anterior. A substituição numa PS4 real ainda precisa de confirmação.
Consulte as [notas da versão](RELEASE_NOTES_0.1.26.md).

## Atualização 0.1.25 — download das atualizações

Corrige o erro 0x80431073 causado pelo tamanho dos cabeçalhos dos
redirecionamentos de GitHub Releases. Esta versão também disponibiliza
o PKG pelo endereço raw do repositório para as apps antigas conseguirem
descarregar a correção. A instalação requer confirmação no telemóvel.
Consulte as [notas da versão](RELEASE_NOTES_0.1.25.md).

## Atualização 0.1.24 — espaço livre do disco interno

A leitura do espaço livre foi confirmada pelo utilizador numa PS4 13.50 com
GoldHEN. Corrige a recusa de permissão que deixava a app a mostrar
«Medição indisponível». Os torrents continuam em `/data/pkg`.

Inclui os ajustes do instalador e de links PKG das versões anteriores, código
de emparelhamento de quatro números e interface no telemóvel e na televisão.
A consulta ao GitHub continua assinada com a chave pública original: versões
compatíveis mostram o aviso ao abrir a app. A instalação requer confirmação
em **Atualizações da app**; não é silenciosa. Consulte [UPDATES.md](UPDATES.md)
e as [notas da versão](RELEASE_NOTES_0.1.24.md).

Build experimental para PS4 real. O download de links e a medição de espaço
foram confirmados pelo utilizador; o processo completo de substituir a app
através do atualizador ainda precisa de confirmação na consola.

## Histórico

## Atualização 0.1.10 — verificações e atualizações

A app consulta o feed assinado do GitHub ao abrir numa PS4 real. A área
**Atualizações da app** mostra a versão disponível, notas e progresso; pede
confirmação antes de enviar o PKG verificado para as Transferências. A
assinatura Ed25519, SHA-512, identidade do PKG e `APP_VER` são confirmados
antes de qualquer instalação. Torrents e PKG existentes são preservados.

É uma build experimental: a ligação ao GitHub e a substituição da app em
execução precisam de confirmação numa PS4 real com GoldHEN. Windows e shadPS4
apresentam a interface, mas não instalam atualizações. Consulta [UPDATES.md](UPDATES.md)
para publicar versões novas e manter a chave privada fora do projeto.

## Atualização 0.1.9 — interface renovada

Ecrã da televisão com o logótipo, texto suavizado, progresso, estados de erro,
QR numa área própria e espaço livre separado do tamanho da transferência.
O ecrã é desenhado pelo processador; não precisa de fontes extra na consola.
O controlo da aplicação continua na página do telemóvel, aberta pelo QR.

Página adaptada a telemóvel e computador, com separadores Torrent / Link PKG,
arrastar e largar um torrent, pausa/retoma, confirmação de limpeza, lista de
ficheiros, velocidade estimada e tempo restante. A velocidade é calculada
pela diferença entre leituras reais do progresso, sem estimativas fictícias.
Os botões ficam desativados durante pedidos e operações incompatíveis.
O estado do link distingue verificação, envio e erro, incluindo falhas
de arranque depois do registo de uma tarefa.

A pré-visualização local pode ser iniciada com
`python tests/ui_preview.py` e aberta em `http://127.0.0.1:8899/`.
Requer Python e OpenCV (`opencv-python`). É apenas uma demonstração
com dados sintéticos: nunca descarrega nem instala ficheiros.
O servidor real continua na porta 8787 e usa o código apresentado na PS4.

Validação desta versão: compilação PS4 e shadPS4, verificação dos dois PKG,
67 casos de validação/envio, testes do JavaScript real com rede controlada,
navegador a 1440 e 390 px e oito estados desenhados pelo renderer C.
Os QR dos sete estados com rede foram lidos corretamente.
O envio, instalação e repouso em consola real continuam por testar.
SFO 00.19, TITLE_ID HBRW00001, destino dos torrents /data/pkg.

O atlas gráfico está incluído em `src/ui_assets.h`. A regeneração opcional
usa `tools/make_ui_assets.py`, Pillow e as fontes Segoe UI instaladas no
Windows. Os ficheiros de fonte não são incluídos no pacote.

## Atualização 0.1.8 — links PKG e ecrã de progresso

A página permite enviar um link direto HTTP/HTTPS para download e instalação
nas Transferências da PS4 (BGFT). A verificação lê só o cabeçalho do PKG,
confirma o tamanho de 64 bits e exige suporte HTTP Range. Páginas de login,
redirecionamentos, PKG divididos, DLC e delta PKG são recusados nesta versão.
Os certificados HTTPS não são desativados. O link original, incluindo a query,
é entregue ao sistema, sem depender de um servidor ou proxy dentro da app.
Uma aplicação base já instalada nunca é desinstalada automaticamente.

O pedido é verificado numa tarefa separada, com estado próprio na página.
Só há mensagem de envio depois do registo e arranque no BGFT. Isto não prova
que o ficheiro tenha sido descarregado ou instalado: acompanha-o em
Notificações → Transferências. Se o arranque falhar após o registo, a mensagem
indica o número da tarefa existente para evitar um reenvio involuntário.
O pedido pertence então à PS4; limpar um torrent não cancela as Transferências.

Para testar repouso: confirma primeiro que o download começou nas Transferências
e ativa «Permanecer ligado à Internet» nas funcionalidades de repouso da PS4.
O servidor tem de permanecer acessível. Compatibilidade com repouso e GoldHEN
ainda não verificada numa consola real; não é uma garantia desta build.
O modo direto instala pela PS4 e não guarda um PKG em `/data/pkg`. Os torrents
continuam a usar essa pasta e requerem a app aberta e a consola ligada.
Windows e shadPS4 mostram a opção, mas recusam o envio para o sistema.

No ecrã da televisão, o estado vazio indica «Nenhum torrent carregado».
O espaço livre tem agora uma linha própria, atualizada a cada 5 segundos.
Quando não é possível medi-lo, mostra «medição indisponível», sem inventar 0 GB.
A versão SFO é 00.18 e o TITLE_ID continua a ser HBRW00001.

Referências de implementação: [BGFT ABI](https://github.com/flatz/ps4_stub_lib_maker_v2/blob/master/include/bgft.h),
[registo de links no ezRemote](https://github.com/cy33hc/ps4-ezremote-client/blob/master/source/installer.cpp),
[campos do PKG](https://github.com/flatz/ps4_remote_pkg_installer/blob/master/pkg.h),
[definições de repouso da PlayStation](https://www.playstation.com/pt-pt/support/hardware/power-saving/).

Testes automatizados desta revisão: `tests/pkg_validation_wasm.js` executa o
código C de validação com um PKG gerado, entradas inválidas e tamanhos >4 GB;
`tests/web_link.js` executa o script real da página com DOM/rede controlados;
`tests/remote_queue_wasm.js` executa `ps4_remote.c` em WebAssembly de 64 bits
com chamadas PS4 controladas, verificando registo, falhas, proteção de títulos
instalados e libertação dos recursos HTTP. São 42 casos de validação de
URL/PKG/Range, 25 casos de envio e verificações dos estados da página.
Estes testes não substituem uma execução de BGFT e repouso na consola.

## Atualização 0.1.7 — nome e ícone

Nome visível alterado de Harbor para h1pNoise na página, no ecrã da consola,
nas notificações e no título do pacote. O TITLE_ID HBRW00001 mantém-se.
Ícone substituído pelo logótipo fornecido pelo utilizador, redimensionado
para PNG de 512 × 512 sem recorte. O original está guardado em
`assets/h1pNoise-original.png`; o ícone do pacote é `assets/icon0.png`.
As versões PS4 e shadPS4 mostram 0.1.7. O destino continua a ser `/data/pkg`.
Esta alteração de apresentação não acrescenta downloads em modo de repouso.

Validação 0.1.7: as duas variantes compilaram sem avisos e passaram a
verificação de hashes e assinaturas do PKG. O título h1pNoise, a versão
00.17 e o ícone foram lidos dos pacotes finais e confirmados. Execução
desta revisão na consola e no emulador ainda por confirmar.

## Atualização 0.1.6 — espaço desconhecido no shadPS4

Apenas no build `--shadps4`, uma consulta de espaço não suportada deixa de
bloquear o download. A API continua a devolver `free: null` e passa a indicar
`allowUnknownSpace: true`. O ecrã e a página explicam essa limitação.
Uma medição válida que indique espaço insuficiente continua a bloquear.
Erros de escrita ou de fecho do ficheiro continuam a interromper o download.
O destino mantém-se em `/data/pkg/<hash>/fileNN.pkg`.

A versão para PS4 real mantém a verificação obrigatória. Esta revisão foi
empacotada para o emulador; não foi executada no shadPS4 por nós. Os dois
caminhos de decisão foram testados executando `storage.c` compilado para
WebAssembly, com consultas de disco controladas (tests/storage_wasm.js).

## Atualização 0.1.5 — pasta de downloads

As versões PS4 e shadPS4 usam agora `/data/pkg` como pasta principal.
Cada torrent mantém uma subpasta identificada pelo seu hash para evitar
colisões: `/data/pkg/<hash>/file00.pkg`, `file01.pkg`, etc.
Os metadados de retoma ficam nessa mesma árvore. A verificação de espaço
e a instalação acompanham o novo destino. O ecrã e a página mostram a pasta.

Os ficheiros antigos em `/data/harbor` não são movidos nem apagados.
É necessário importar novamente o torrent; esta versão não retoma
automaticamente ficheiros guardados na pasta antiga.
No teste Windows, o destino continua a ser o argumento fornecido ao executável.

## Atualização 0.1.4 — medição de espaço

A inspeção do binário ligado com OpenOrbis 0.5.3 mostrou que `statvfs()`
não copia os campos para a estrutura fornecida pelo chamador. A estrutura
inicializada a zero fazia o Harbor interpretar uma consulta sem resultado
como disco cheio. O wrapper `statfs()` também não preserva o retorno de
`fstatfs()`. A versão PS4 passa agora por `open` + `fstatfs` + `close`, verifica
o resultado e calcula os bytes a partir de blocos com campos de 64 bits.

Uma consulta falhada ou uma estrutura inválida produz `free: null`, não zero.
O download continua bloqueado se não for possível medir o espaço, com uma
mensagem explícita. Quando falta espaço de facto, indica o destino, o espaço
disponível e o necessário, incluindo a margem existente de 64 MiB.
A instalação usa a mesma distinção. Sem torrent carregado, a página deixa
de mostrar o ambíguo contador 0 / 0 e indica que não há torrent carregado.

O destino PS4 permanece `/data/harbor`. Esta revisão não muda o download
para um disco USB e não desativa a proteção contra falta de espaço.
A correção ainda precisa de confirmação numa PS4 física.

Validação 0.1.4: 13 testes de integração Windows e 12 verificações unitárias
de armazenamento passaram. A versão PS4 compilou; o binário foi inspecionado
para confirmar a chamada direta a `fstatfs` e a preservação do seu retorno.

Teste unitário de armazenamento (além de `tests/integration.py`):
`zig cc tests/storage.c src/storage.c -o build/test-storage.exe`
seguido de `build/test-storage.exe`.

## Atualização 0.1.3

QR real no ecrã da consola, gerado localmente com Nayuki v1.8.0 (MIT).
A leitura abre `http://IP:8787/#code=CODIGO`; a página preenche o código,
remove-o da barra de endereço e tenta ligar. Não usa serviços externos de QR.
Sem IP válido, o ecrã pede ligação à rede em vez de mostrar um QR inválido.

Build específico para shadPS4: acrescentar `--shadps4` ao comando de compilação.
Produz `build-shadps4/Harbor-0.1.3-shadPS4-test.pkg`. O shadPS4 v0.18.0
(e3ce810) encaminha as opções POSIX de socket para a enumeração SceNet:
`SO_RCVTIMEO=0x1006` causa a asserção `net.h:167 NameOf`. Só neste build usamos
0x1106/0x1105 e um inteiro em microssegundos, como o emulador espera.
A versão normal PS4 mantém a ABI POSIX. Instalação de PKG é explicitamente
indisponível no build shadPS4, pois BGFT/AppInstUtil aparecem como stubs.

No emulador, ativar a opção de ligação à rede e reiniciar a aplicação.
O registo enviado tinha `isConnectedToNetwork: false`; nessa configuração
não é possível obter o IP através de NetCtl. Não é necessário ativar shadNet
para esta ligação HTTP local. Outras funções de rede/disco ainda podem estar
incompletas no emulador. Esta atualização não foi executada por nós no shadPS4.

Verificação desta revisão: 11 testes Windows com dados sintéticos passaram,
incluindo leitura do QR produzido pelo código C através de OpenCV. As duas
variantes PS4 compilaram e passaram a validação de integridade do PKG.
Os testes precisam de `opencv-python` e `numpy` para a leitura independente.

Aplicação homebrew empacotada como FPKG. Recebe um ficheiro `.torrent` através
de uma página local, descarrega PKG na PS4 e disponibiliza instalação sequencial.

**Estado: experimental. Compilada para PS4 com OpenOrbis, mas ainda não executada
numa PS4 real. A compatibilidade com firmware 13.52 / GoldHEN v2.4b18.11 e a
instalação BGFT continuam por validar na consola.** Os testes Windows verificam
o motor de download e o servidor; não emulam a PS4.

## Funcionalidades implementadas

- Upload de `.torrent` pelo navegador de um telemóvel na mesma rede.
- Código de acesso aleatório por execução; controlo de Host e Origin.
- Torrents BitTorrent v1 com 1 a 32 ficheiros `.pkg` diretos.
- Trackers HTTP e UDP; peers TCP/IPv4, até quatro ligações em simultâneo.
- Verificação SHA-1 de cada peça antes de gravar os dados recebidos.
- Download de vários ficheiros, incluindo peças que atravessam a fronteira entre ficheiros.
- Pausa e retoma; nova verificação do conteúdo existente depois de reiniciar.
- Tamanhos e offsets de ficheiros de 64 bits.
- Instalação local BGFT: bases antes das atualizações, segundo os flags dos PKG.
- Não desinstala jogos existentes e mantém os PKG após a instalação.

## Limites desta versão

Não inclui login no Telegram, pesquisa no bot, magnets, DHT, PEX, uTP, IPv6,
trackers HTTPS, redirects HTTP, encriptação do protocolo BitTorrent, seeding,
RAR/ZIP, seleção individual de ficheiros ou eliminação pela interface.
Torrents dependentes dessas funcionalidades poderão não funcionar. Um tracker
inacessível ou a ausência de peers pode deixar o download à espera.

Só trata um torrent de cada vez. Os ficheiros ficam em
`/data/pkg/<infohash>/file00.pkg`, `file01.pkg`, etc.; os nomes originais
aparecem na página, mas não são utilizados como caminhos no disco.

Um jogo base já instalado interrompe a instalação automática, sem o remover.
Uma atualização exige o jogo base correspondente. A instalação não foi testada
em hardware; em caso de erro, conserva os ficheiros e apresenta o código recebido.

## Compilar

Requisitos para Windows: Python 3, LLVM 18.1.8, OpenOrbis 0.5.3 (build LLVM 18)
e runtime .NET compatível. O script permite roll-forward para um runtime .NET
mais recente. Não requer o SDK proprietário da Sony.

```powershell
python build.py --sdk C:/caminho/PS4Toolchain --llvm C:/caminho/LLVM/bin
```

Saída: `build/h1pNoise-0.1.9-experimental.pkg` ou, com `--shadps4`,
`build-shadps4/h1pNoise-0.1.9-shadPS4-test.pkg`. O script atualiza o HTML embutido,
compila o ELF, produz o SELF, cria os metadados SFO e empacota o FPKG.

Para os testes do motor no Windows, usar Zig 0.13.0:

```powershell
python build.py --host --zig C:/caminho/zig.exe
python tests/integration.py
```

Os testes criam apenas um tracker e um peer locais com dados artificiais.
Não contactam trackers públicos nem descarregam jogos. A implementação da
instalação no Windows devolve indisponível; apenas o build PS4 utiliza BGFT.

## Validação realizada

10 testes locais passaram: autenticação/HTTP, torrents malformados, download
multifile e integridade, retoma com tracker chunked, reabertura, rejeição de
cabeçalho PKG inválido, pausa, tracker UDP, rejeição de peça corrompida e
metadados de 52 264 632 320 bytes.

A página foi inspecionada num navegador com viewport de 390 × 844. O torrent
fornecido pelo utilizador foi importado para confirmar nomes e tamanhos,
sem iniciar qualquer download. Esse torrent não está incluído nesta distribuição.

## Referências

- [OpenOrbis 0.5.3](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/releases/tag/v0.5.3)
- [BGFT / Remote Package Installer](https://github.com/flatz/ps4_remote_pkg_installer)
- [Instalação local no ezRemote Client](https://github.com/cy33hc/ps4-ezremote-client/blob/master/source/installer.cpp)
- [Fonte bitmap pública font8x8](https://github.com/dhepper/font8x8)

O ABI de progresso BGFT usa contadores de 64 bits. O código corrige localmente
a estrutura de 32 bits presente no cabeçalho do OpenOrbis utilizado, mantendo
o layout de 64 bytes documentado pelo projeto de referência.
