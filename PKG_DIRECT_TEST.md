# Teste: instalar a atualização mantendo a h1pNoise aberta

Este canal testa **0.1.38 → 0.1.39**. O utilizador confirmou que conseguiu
instalar manualmente a 0.1.33 sobre a 0.1.32 com a app aberta, usando o GoldHEN
com **Enable Background Installation desligado**. Isso não confirma ainda
a chamada automática de instalação feita pela própria app.

1. Instala `h1pNoise-0.1.38-direct-test.pkg` pelo GoldHEN, com Background
   Installation desligado, sem desinstalar a versão anterior.
2. Abre a h1pNoise, liga o telemóvel pelo QR e procura uma atualização.
3. Escolhe **Descarregar e instalar sem fechar (teste)**. O canal exclusivo
   anuncia a 0.1.39. Não é necessário instalar nem abrir o Updater auxiliar.
4. Se o sistema aceitar e o pacote instalado for confirmado, a app indica
   **Nova versão instalada** e continua aberta. A versão em execução continua
   a ser 0.1.38 até voltares a abrir a app; nessa altura deve indicar 0.1.39.

Se falhar, envia `/data/pkg/update-install-debug.log` e
`/data/pkg/link-debug.log`. O original fica em
`/data/pkg/h1pNoise-update-49.pkg` para instalação manual. Não repitas nem
elimines o pedido guardado antes de analisar o registo: uma chamada pode ter
sido aceite sem confirmação, ou a app pode ter fechado durante a chamada.

## Alterações após o teste anterior

Na PS4 real, a tentativa 0.1.36 → 0.1.37 devolveu zero da API e o hash do
PKG instalado foi confirmado. Porém a versão instalada não abriu e apresentou
CE-30012-6. A confirmação do ficheiro não prova que o pacote possa abrir.

A cópia comum usada nesse teste não reservava blocos do disco antes da escrita.
O método documentado por flatz descreve este problema: a instalação direta
move o ficheiro para a pasta da app, mas o PFS pode falhar ao abrir se a cópia
não tiver sido preparada no disco. Esta é uma hipótese para o erro observado,
não uma causa confirmada apenas pelo código CE-30012-6.

Este novo teste configura slot/prioridade e reserva os blocos usando os IOCTL
documentados, antes de escrever a cópia. Se o dispositivo ou um comando não
forem acessíveis/suportados, a operação pára antes da instalação e conserva
o download original. A reserva incide apenas no descritor da cópia nova;
nenhum comando é aplicado ao ficheiro da app instalada. As permissões
temporárias são restauradas em cada operação.

A resolução exige confirmar tanto a instalação como a abertura da 0.1.39 numa
PS4 real. O método direto também pode exigir integração com o BGFT ao abrir;
este teste não altera o ShellCore nem aplica offsets de firmware de exemplos
antigos. Se a reserva passar mas a app continuar sem abrir, o registo e a
recuperação manual são necessários antes de tentar outra instalação automática.

O canal antigo `releases/pkg-direct-test/current.h1p` foi suspenso, anunciando
um build anterior para impedir que as versões 0.1.34/0.1.36 ofereçam mais
atualizações pelo método antigo. Os seus PKG públicos não foram alterados.
Instala primeiro a 0.1.38 pelo GoldHEN: recuperar a app e trocar para o canal
novo faz parte deste teste.

## Implementação e limites

- Flag exclusiva `HARBOR_PKG_DIRECT_TEST`, opção `build.py --pkg-direct-test`.
- Canal `releases/pkg-direct-allocated-test/current.h1p`. Não altera os canais manual,
  legacy, runtime ou o teste antigo com Updater. O teste direto antigo foi
  suspenso por falha de abertura.
- Verifica assinatura Ed25519, SHA-512, tamanho e identidade/SFO do PKG.
- Confirma o PKG atualmente instalado no disco interno e recusa patches,
  versões iguais/anteriores e instalações inacessíveis. Disco externo não foi
  validado neste teste.
- Prepara os blocos do disco e verifica uma cópia `.install.pkg`, conservando
  o original. Regista
  uma tentativa durável antes de chamar `sceAppInstUtilAppInstallPkg`.
- Não chama PrepareOverwrite, Uninstall, BGFT, LaunchApp, LoadExec ou exit.
- Restaura as permissões depois da chamada. Verifica o pacote instalado;
  um resultado zero da API, por si só, não é apresentado como sucesso.
- Não altera a opção Background Installation do GoldHEN. A API pode recusar
  a substituição; o teste não acrescenta uma desinstalação como alternativa.
- A confirmação das chamadas reais e da permanência da app aberta exige PS4.
  Os testes de PC usam chamadas de sistema simuladas; shadPS4 não valida isso.
- Depois de uma tentativa enviada, não repete automaticamente a instalação.
  Uma versão nova em execução pode concluir o pedido anterior assinado.

Referência da API nativa: https://flatz.github.io/ . A chamada de preparação
de substituição desse exemplo foi deliberadamente omitida neste teste.
