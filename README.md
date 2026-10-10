<img width="1891" height="899" alt="Captura de ecrã 2026-10-06 211809" src="https://github.com/user-attachments/assets/e4683b8f-83e9-4e46-90a7-61325069773b" />
<img width="1842" height="879" alt="Captura de ecrã 2026-10-06 211815" src="https://github.com/user-attachments/assets/d5c4277a-0e6d-482c-801c-fc0fb5dff194" />
<img width="1855" height="881" alt="Captura de ecrã 2026-10-06 211823" src="https://github.com/user-attachments/assets/fd71aa14-c3d8-4131-b630-5b6ada304648" />




# h1pNoise — PS4

Central de transferências para PS4 com GoldHEN. Envia torrents, magnets ou links diretos PKG pela interface web no telemóvel ou computador, e acompanha o progresso na consola.

## Funcionalidades

- Ligação por QR ou código de quatro números, na mesma rede.
- Torrents e magnets: download, pausa, retoma e instalação de PKG.
- Real-Debrid para torrents e magnets, com aba própria e API/preferência guardadas na consola.
- Links diretos PKG enviados às Transferências da PS4.
- Espaço livre do disco interno e torrents guardados em `/data/pkg`.
- Atualizações assinadas guardadas no disco interno ou na raiz da pen USB.
- Limpeza das atualizações antigas, preservando os outros PKG.
- Cor personalizável e sincronizada entre a consola e a interface web.

## Instalar

Descarrega [h1pNoise-1.0.1.pkg](https://github.com/h1pNoise/h1pNoise-ps4/releases/download/1.0.1/h1pNoise-1.0.1.pkg). Fecha a app e instala pelo Package Installer do GoldHEN, com **Enable Background Installation desligado**. Depois abre a h1pNoise e liga-te pelo QR ou endereço apresentado na televisão.

## Atualizar

Usa **Procurar atualização**, escolhe disco interno ou pen exFAT/FAT32 e descarrega o PKG. Os nomes seguem a versão: `h1pNoise-1.0.0.pkg`, `h1pNoise-1.0.1.pkg`, etc. Fecha a app e instala manualmente pelo GoldHEN. **Apagar updates antigas** remove apenas ficheiros de atualização identificados e verificados pela app.

## Compatibilidade

Compatibilidade reportada: **PS4 13.50 com GoldHEN**. Para torrents e magnets, mantém a app aberta e a PS4 ligada. Sem Real-Debrid, magnets precisam de tracker HTTP/UDP ou fonte direta suportada; sem DHT/PEX. O repouso para links PKG ainda não foi confirmado. Esta revisão foi compilada e testada no PC; ainda sem novo teste físico na consola.

O Real-Debrid exige conta Premium e API próprias. Aceita até 32 PKG diretos por magnet, incluindo subpastas; ignora imagens/NFO e outros ficheiros extra. ZIP/RAR e partes não são descompactados. [Configurar Real-Debrid](REAL_DEBRID.md).

Código sob [GPL-3.0](LICENSE). [Créditos](NOTICE.md) · [Publicar atualizações](UPDATES.md).

## Conteúdo do repositório

Inclui o código da aplicação PS4, a interface web integrada no PKG, os recursos visuais, a ferramenta de compilação e os testes da versão atual. O download para instalar na consola está na [release 1.0.1](https://github.com/h1pNoise/h1pNoise-ps4/releases/tag/1.0.1).
