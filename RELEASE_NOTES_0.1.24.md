# h1pNoise 0.1.24 — leitura do espaço livre do disco interno

O storage-debug.log da 0.1.23 confirmou que /data/pkg abre, mas tanto a
consulta direta ao sistema como a biblioteca devolvem EPERM (permissão
recusada). A resposta vazia não é evidência de que o disco esteja cheio.

Quando a consulta recebe esse erro, esta versão tenta novamente no mesmo
descritor com as credenciais e o contexto de acesso temporariamente elevados
através do libjbc já usado pelo instalador. Mantém os três diretórios raiz
originais. O libjbc só abre descritores de substituição quando esses diretórios
mudam; uma consulta de armazenamento não os altera temporariamente.
Restaura as credenciais guardadas antes de apresentar qualquer medição,
incluindo quando a ativação ou a consulta falham. Se a restauração falhar,
não volta a tentar elevar o acesso durante essa execução.

Um bloqueio partilhado evita sobrepor esta leitura às alterações de acesso
do instalador. A consulta não espera pelo instalador enquanto outro bloqueio
da aplicação está ocupado. O destino dos downloads continua em /data/pkg.
O valor representa o espaço disponível no sistema de ficheiros do disco que
contém essa pasta, não o tamanho ou uma quota atribuída à pasta.

Respostas inválidas continuam a aparecer como «Medição indisponível».
Essa situação não impede o download; falta real de espaço e erros de escrita
continuam a ser tratados. Links PKG mantêm o comportamento da 0.1.22.

O registo /data/pkg/storage-debug.log acrescenta os resultados de guardar,
resolver, ativar, consultar e restaurar o acesso. Não regista códigos de
emparelhamento, links ou tokens. Instalar por cima da versão anterior, após
terminarem os downloads/instalações, e voltar a abrir a aplicação.

Verificação: compilação OpenOrbis; 28 casos de consulta PS4, 16 de shadPS4,
políticas de armazenamento nas três plataformas, 18 casos do acesso ao
instalador e 23 casos do libjbc em memória simulada. Estes últimos verificam
o caminho que conserva as raízes, ausência de aberturas de substituição,
restauração integral e equilíbrio das referências. Passaram ainda 43 casos
da fila PS4, validação do PKG, interface web e 118 casos do atualizador.
Os testes de permissões simulam o kernel. Depois deste teste, o utilizador
confirmou a medição na PS4 real 13.50 com GoldHEN: 302 GB livres.
APP_VER 00.34, TITLE_ID HBRW00001. Build experimental.
