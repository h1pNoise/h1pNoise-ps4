# h1pNoise 0.1.20 — teste local PS4

Corrige a falta de inicialização do UserService antes de consultar o
utilizador ativo. O erro 0x80960002 corresponde a NOT_INITIALIZED, e a
mensagem anterior que pedia selecionar um utilizador era incorreta.

O serviço é carregado e inicializado com a prioridade normal 0x2BC.
A resposta ALREADY_INITIALIZED é aceite. IDs inválidos ou o ID do sistema
impedem o registo de tarefas. A mensagem para selecionar um utilizador
passa a ser reservada para NOT_LOGGED_IN ou um ID inválido. Outras falhas
mantêm o código e distinguem a inicialização da consulta.

A função é partilhada pela fila de links PKG e pela instalação de
atualizações da h1pNoise. Os resultados das chamadas ficam no link-debug.log.
Mantém o código de quatro números, downloads de torrents em /data/pkg
e a correção de acesso ao AppInstUtil/BGFT da 0.1.19.

O registo recebido da consola confirma que a 0.1.19 carregou AppInstUtil
e BGFT, inicializou ambos e restaurou as credenciais anteriores com sucesso.
A transferência do link ainda não foi confirmada na consola.

Verificação: compilação OpenOrbis concluída; 42 testes de PKG, 32 testes da
fila PS4, 9 do controlo de acesso temporário, 16 do resolvedor de credenciais,
108 do sistema de atualizações e testes web. Os serviços da PS4 são simulados
nos testes. O mock do serviço de utilizadores devolve NOT_INITIALIZED até
à inicialização, para reproduzir esta falha.

Versão experimental local, sem publicação no GitHub.
