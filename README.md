# h1pNoise — PlayStation 4

Central de transferências para PS4 com GoldHEN. Liga-te pela app ou pelo site, usando o QR ou o código de quatro números para enviar torrents, magnets e links diretos PKG, acompanhar o progresso e descarregar atualizações da app.

**1.0.0 — versão final.** Inclui torrents, magnets, links PKG, espaço livre do disco interno, destinos de atualização interno/USB, limpeza das atualizações antigas e cores sincronizadas entre a televisão e a app ou o site. Mantém o funcionamento da 0.1.60. A compatibilidade reportada pelo utilizador é PS4 13.50 com GoldHEN; outros firmwares não foram confirmados. A instalação das atualizações continua manual pelo GoldHEN.

## Instalar e ligar

1. Descarrega o PKG em [Releases](https://github.com/h1pNoise/h1pNoise-ps4/releases).
2. Fecha a h1pNoise. No Package Installer do GoldHEN, desliga **Enable Background Installation** e instala o PKG. Aceita substituir se for pedido; não desinstales primeiro.
3. Abre a app na PS4. Liga o telemóvel ou computador à mesma rede da consola. Na app do telemóvel, usa o QR ou o endereço e o código. Num navegador do telemóvel ou computador, abre o endereço apresentado e introduz o código de quatro números.
4. O nome e a versão aparecem na televisão e na app ou no site.

## Transferências

O botão **Cor**, no topo da página, muda a cor dos botões, barras e detalhes. Escolhe uma das cores disponíveis ou uma cor personalizada. Depois de ligar à consola, a escolha fica guardada na PS4 e muda também as cores da televisão. Outros dispositivos ligados à mesma consola recebem a mesma cor. Sem ligação, a escolha fica apenas neste navegador. **Voltar ao verde** restaura a cor original.

- **Torrent:** envia um ficheiro .torrent de até 8 MB. Escolhe Só descarregar ou Descarregar e instalar. Mantém a app aberta e a PS4 ligada.
- **Magnet:** cola uma ligação BitTorrent v1 com tracker HTTP/UDP ou fonte direta suportada. A app obtém e verifica os metadados antes de permitir o download. Também requer a app aberta e a consola ligada.
- **Link PKG:** envia uma ligação direta HTTP/HTTPS para registar o pedido nas Transferências da PS4. Confirma o início em Notificações → Transferências. O servidor precisa de disponibilizar o ficheiro diretamente e o link não pode expirar durante o download.
- **Pausa e retoma:** o conteúdo existente volta a ser verificado depois de reabrir. Limpar e reiniciar prepara outra transferência e mantém os ficheiros descarregados.

Os torrents ficam em `/data/pkg/<infohash>/file00.pkg`, `file01.pkg`, etc. Os nomes originais aparecem na interface. O download por link usa as Transferências da PS4 e não guarda uma cópia adicional em /data/pkg.

## Atualizações da app

Ao abrir ou escolher **Procurar atualização**, a app consulta o canal assinado. Em **Descarregar atualização**, escolhe disco interno (/data/pkg) ou a raiz de uma pen exFAT/FAT32 ligada à PS4. Não uses um disco configurado como armazenamento expandido para este destino.

O ficheiro `h1pNoise-update-<build>.pkg` é verificado antes de ser anunciado como pronto. Depois fecha a app e instala pelo GoldHEN, com a instalação em segundo plano desligada. A app não se desinstala nem lança um payload para se atualizar.

**Apagar updates antigas** pede destino e confirmação. Apaga apenas os nomes fixos h1pNoise-update de builds anteriores à instalada. Preserva a instalada, todas as mais recentes, os outros PKG, os torrents e as subpastas. O resultado indica quantos ficheiros foram apagados.

## Limites conhecidos

- Um torrent ou magnet de cada vez, com conteúdo PKG direto; sem ZIP/RAR.
- Sem DHT, PEX, uTP, IPv6, trackers HTTPS ou torrents apenas v2. Um magnet precisa de uma fonte suportada.
- O repouso para torrents não é suportado. O comportamento dos links PKG em repouso com GoldHEN ainda precisa de teste próprio.
- O shadPS4 é um alvo de teste separado: não confirma a instalação BGFT nem as atualizações numa consola real.

## Compilar e verificar

Windows, Python 3, LLVM e OpenOrbis PS4 Toolchain:

```powershell
python build.py --sdk C:/caminho/PS4Toolchain --llvm C:/caminho/LLVM/bin
```

As versões 0.x produzem `build/h1pNoise-<versão>-experimental.pkg`; versões 1.x ou superiores produzem `build/h1pNoise-<versão>.pkg`. O alvo `--shadps4` continua separado. Mantém TITLE_ID HBRW00001 e CONTENT_ID IV0000-HBRW00001_00-HARBORPS40000000 nas atualizações.

Os testes Windows usam dados artificiais e servidores locais:

```powershell
python build.py --host --zig C:/caminho/zig.exe
python tests/integration.py
node tests/web_link.js web/index.html
```

Os testes do atualizador executam a assinatura, SHA-512, parser e fluxo de produção, simulando as chamadas PS4. A lista de verificação na consola está em [TESTE_FINAL.md](TESTE_FINAL.md). A publicação do canal está em [UPDATES.md](UPDATES.md).

## Referências

- [OpenOrbis](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain)
- [BGFT / Remote Package Installer](https://github.com/flatz/ps4_remote_pkg_installer)
- [Monocypher](https://monocypher.org/)
- [QR Code generator](https://github.com/nayuki/QR-Code-generator)

As notas no repositório preservam o histórico dos testes e das correções.
