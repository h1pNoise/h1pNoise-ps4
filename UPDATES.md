# Publicar atualizações

A app consulta o manifesto assinado em `releases/manual-v1/current.h1p`. Mantém esse endereço permanente e usa a mesma chave pública na app. A chave privada de publicação fica apenas no computador do responsável e nunca deve ser enviada ao GitHub.

Cada publicação precisa de uma versão numérica (`1.0.0`, `1.0.1`, etc.), um APP_BUILD superior ao anterior, uma versão PS4 APP_VER superior e a identidade HBRW00001 / IV0000-HBRW00001_00-HARBORPS40000000 preservada. O PKG deve conter o número de versão correto; renomear o ficheiro não altera a app.

1. Compila e testa o PKG com `python build.py --sdk <PS4Toolchain> --llvm <LLVM/bin>`.
2. Publica uma release estável marcada Latest, com o ficheiro `h1pNoise-<versão>.pkg` e o resumo da app.
3. Confirma o tamanho e SHA-256 do ficheiro publicado.
4. Usa `tools/release_update.py` para gerar o manifesto assinado do canal `manual-v1`, com o URL direto do PKG.
5. Verifica a assinatura, SHA-512 e metadados do pacote antes de atualizar `releases/manual-v1/current.h1p`.

Na app, **Procurar atualização** compara o APP_BUILD. **Descarregar atualização** permite `/data/pkg`, `/mnt/usb0` ou `/mnt/usb1`; o ficheiro fica diretamente na raiz da pen. A assinatura, o hash e a identidade do PKG são verificados. A instalação é manual pelo GoldHEN, com a app fechada e Enable Background Installation desligado.

Os downloads usam `h1pNoise-<versão>.pkg`. A app regista os metadados assinados em `.h1pNoise-update-<build>.h1p`. A limpeza exige um registo válido de uma build anterior e um PKG cujo conteúdo ainda corresponda ao hash assinado. Ficheiros alterados, não registados, de builds atuais ou futuras, outros PKG e subpastas ficam preservados. O suporte aos nomes internos antigos continua no código para limpar ficheiros já descarregados.

O GitHub pode servir uma cópia em cache do canal durante alguns minutos. Depois de publicar, reabre a app e repete a pesquisa de atualização se necessário.
