# h1pNoise 0.1.21 — teste local PS4

Tentativa de resolver o erro 0x80990007 no registo da transferência BGFT.
O registo da 0.1.20 confirma que os módulos, os serviços de instalação e
a consulta do utilizador já tinham concluído. As permissões temporárias
tinham sido restauradas antes da chamada de registo.

A nova submissão usa temporariamente o auth ID ShellCore e a capacidade
system durante o registo e o arranque da tarefa. A auth info anterior é
reposta em todos os caminhos de retorno. Esta alteração não muda UID,
prison, diretórios raiz ou referências de vnodes. A função de acesso aos
módulos continua separada da autorização para submeter tarefas.
Referência técnica: https://flatz.github.io/

O caminho é partilhado pelos links PKG, atualizações da app e instalação
de ficheiros locais. Os códigos devolvidos pelo registo, arranque e
reposição de permissões ficam no link-debug.log. 0x80990007 passa a mostrar
uma mensagem de permissões, em vez de sugerir falta de espaço.

Se o registo tiver sucesso, o número da tarefa fica guardado antes do
arranque e continua disponível se o arranque ou a reposição falharem.
Não há nova tentativa automática de registo nem remoção de tarefas ou
aplicações existentes.

Verificação: compilação OpenOrbis; 42 testes PKG, 38 da fila PS4, 16 do
acesso temporário, 21 do resolvedor/auth-info, 118 das atualizações e testes
web. Os testes exigem as permissões durante as chamadas BGFT, verificam
a reposição após falhas e a preservação dos pedidos já registados.
A implementação auth-only também foi executada sobre um mapa artificial
de memória e a reposição exata dos campos foi verificada.

Os testes simulam os serviços da PS4 e não confirmam a aceitação da tarefa
ou o download/repouso numa PS4 13.50. Requer teste na consola.
Mantém código de quatro números e torrents em /data/pkg.
Versão experimental local, sem publicação no GitHub.
