# Teste de instalação automática de atualizações

Este canal é experimental e precisa de confirmação numa PS4 real. A publicação
normal 0.1.31 e o seu canal `manual-v1` continuam com instalação manual. Os amigos
que usam esse canal não recebem estes PKG de teste.

## Testar 0.1.32 → 0.1.33

1. No GoldHEN, desliga **Enable Background Installation** e fecha a h1pNoise.
2. Instala uma vez `h1pNoise-Updater-0.1.0-test.pkg` e
   `h1pNoise-0.1.32-install-test.pkg`. Não desinstales a h1pNoise antes: esta versão
   usa a mesma identidade da app e uma versão de pacote superior.
3. Abre a **h1pNoise 0.1.32**, liga o telemóvel pelo QR e abre **Atualizações da app**.
4. Carrega em **Procurar atualização** e depois em
   **Descarregar e instalar (teste)**. O canal separado anuncia a 0.1.33.
5. Após descarregar e verificar, a app tenta abrir **h1pNoise Updater**. O auxiliar
   verifica novamente o PKG e a versão instalada, confirma a entrega e aguarda
   que o processo da h1pNoise termine antes de chamar o instalador.
6. O auxiliar só anuncia sucesso depois de verificar o pacote realmente
   instalado. Tenta reabrir a h1pNoise; se não abrir, abre-a no menu da PS4 e
   confirma que apresenta **0.1.33**.

É normal a página do telemóvel perder a ligação quando a h1pNoise fecha. Volta a
ligar pelo QR depois de a app reabrir. Não abras o auxiliar manualmente antes de
existir um pedido: ele depende da entrega criada pela app.

## Se falhar

O PKG descarregado fica em `/data/pkg/h1pNoise-update-43.pkg` e pode ser instalado
pelo GoldHEN com a h1pNoise e o auxiliar fechados. O código não chama a preparação
de substituição nem a desinstalação da app anterior.

Guarda `/data/pkg/update-install-debug.log` e confirma a versão que aparece ao
abrir a app. Um pedido entregue que não ficou confirmado impede novas tentativas
automáticas. Não apagues esse pedido enquanto não for possível determinar pelo
registo se o instalador aceitou a operação.

Este primeiro teste exige que a h1pNoise esteja instalada no disco interno e
que não tenha um pacote de patch separado. Se o auxiliar não conseguir ler o
pacote instalado, recusa a instalação. Não valida instalação no shadPS4, repouso
ou funcionamento em segundo plano.

## Validação feita no PC

- 38 verificações do auxiliar: assinatura, hash, tamanho, identidade, versão,
  processo anterior, falhas do instalador e confirmação do pacote instalado.
- 28 verificações da entrega 0.1.32 → 0.1.33: lançamento separado, pedido guardado,
  confirmação vinculada à entrega, reposição das permissões antes de fechar e
  preservação do PKG nas falhas.
- 89 verificações do canal manual e 113 do protótipo de atualização do executável.
- Verificações dos botões web e compilação dos dois PKG de teste e do auxiliar.

As chamadas do sistema foram simuladas nesses testes. A compatibilidade real
do lançamento do auxiliar e do instalador ainda precisa de ser testada na PS4.

## Compilar

`build.py --pkg-installer-test` gera a 0.1.32 / APP_VER 00.42 e o auxiliar HBRU00001.
`build.py --pkg-installer-test --pkg-installer-version 0.1.33 --pkg-installer-build 43`
gera o candidato 0.1.33 / APP_VER 00.43. Os argumentos normais de SDK e LLVM
continuam necessários.

O canal exclusivo é `releases/pkg-install-test/current.h1p`. O manifesto usa a
mesma assinatura Ed25519, tamanho, SHA-512 e identidade de pacote que os outros
canais. `release_update.py --channel pkg-install-test` recusa PKG normais, e
`--channel manual-v1` recusa PKG com o sufixo `-install-test.pkg`.
