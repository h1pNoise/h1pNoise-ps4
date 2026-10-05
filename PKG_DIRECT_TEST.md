# Teste: instalar a atualização mantendo a h1pNoise aberta

Este canal testa **0.1.34 → 0.1.35**. O utilizador confirmou que conseguiu
instalar manualmente a 0.1.33 sobre a 0.1.32 com a app aberta, usando o GoldHEN
com **Enable Background Installation desligado**. Isso não confirma ainda
a chamada automática de instalação feita pela própria app.

1. Instala `h1pNoise-0.1.34-direct-test.pkg` pelo GoldHEN, com Background
   Installation desligado, sem desinstalar a versão anterior.
2. Abre a h1pNoise, liga o telemóvel pelo QR e procura uma atualização.
3. Escolhe **Descarregar e instalar sem fechar (teste)**. O canal exclusivo
   anuncia a 0.1.35. Não é necessário instalar nem abrir o Updater auxiliar.
4. Se o sistema aceitar e o pacote instalado for confirmado, a app indica
   **Nova versão instalada** e continua aberta. A versão em execução continua
   a ser 0.1.34 até voltares a abrir a app; nessa altura deve indicar 0.1.35.

Se falhar, envia `/data/pkg/update-install-debug.log` e
`/data/pkg/link-debug.log`. O original fica em
`/data/pkg/h1pNoise-update-45.pkg` para instalação manual. Não repitas nem
elimines o pedido guardado antes de analisar o registo: uma chamada pode ter
sido aceite sem confirmação, ou a app pode ter fechado durante a chamada.

## Implementação e limites

- Flag exclusiva `HARBOR_PKG_DIRECT_TEST`, opção `build.py --pkg-direct-test`.
- Canal `releases/pkg-direct-test/current.h1p`. Não altera os canais manual,
  legacy, runtime ou o teste antigo com Updater.
- Verifica assinatura Ed25519, SHA-512, tamanho e identidade/SFO do PKG.
- Confirma o PKG atualmente instalado no disco interno e recusa patches,
  versões iguais/anteriores e instalações inacessíveis. Disco externo não foi
  validado neste teste.
- Cria e verifica uma cópia `.install.pkg`, conservando o original. Regista
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
