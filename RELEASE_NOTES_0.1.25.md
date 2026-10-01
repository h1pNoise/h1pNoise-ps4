# h1pNoise 0.1.25 — download das atualizações

Corrige o erro 0x80431073 ao descarregar uma atualização do GitHub.
Esse código significa que os cabeçalhos da resposta excederam o limite
do cliente HTTP; a mensagem anterior apontava incorretamente para HTTPS.
O redirecionamento do PKG 0.1.24 devolveu 5 226 bytes de cabeçalhos no
teste público, acima do limite padrão de 5 000 bytes.

O atualizador configura agora um limite de 32 KiB no modelo HTTP antes
de criar as ligações e pedidos, incluindo cada redirecionamento.
Respostas acima desse limite continuam a ser recusadas. As mensagens
distinguem cabeçalhos demasiado grandes, falhas HTTPS e redirecionamentos
inválidos. Mantém a validação dos certificados, assinatura Ed25519,
SHA-512, identidade e versão do pacote e confirmação da instalação.

Para permitir que as versões antigas descarreguem esta correção, o
manifesto desta versão aponta para uma cópia do mesmo PKG em
`releases/v0.1.25/` através de raw.githubusercontent.com, sem o
redirecionamento grande de GitHub Releases. O PKG também fica anexado
à release para instalação manual. A chave de assinatura é a original.

Mantém a medição do espaço interno confirmada na 0.1.24 e o destino
`/data/pkg`. Não altera os torrents nem o instalador dos links PKG.

Verificação: compilação PS4 OpenOrbis e 126 casos do atualizador,
incluindo download com redirecionamento de 5 226 bytes, limite exato
de 32 KiB, resposta acima do limite e limpeza após falhas da API.
Estes testes executam o código de produção com APIs PS4 simuladas.
O download e a substituição da app ainda precisam de confirmação
numa consola real. APP_VER 00.35, TITLE_ID HBRW00001. Experimental.
