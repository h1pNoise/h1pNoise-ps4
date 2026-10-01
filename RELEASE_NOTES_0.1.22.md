# h1pNoise 0.1.22 — teste local PS4

Tentativa de resolver CE-32942-0 (BGFT 0x80990033, extensão HTTP recusada)
depois de o pedido ser registado e iniciado. O teste da 0.1.21 na consola
confirmou registo, arranque e reposição de permissões com resultado zero.

Links que não terminem literalmente em .pkg ou .PKG recebem o fragmento
#content.pkg apenas na submissão ao BGFT. O endereço usado para verificar
o cabeçalho continua original. O fragmento não altera o caminho nem os
parâmetros do pedido HTTP; tokens e parâmetros codificados são preservados.
Links já terminados em .pkg/.PKG são mantidos. Endereços que excederiam o
limite de 2048 bytes são recusados antes de contactar o servidor, sem truncar.

Referências do autor do FPKGi:
https://github.com/ItsJokerZz/FPKGi/issues/12#issuecomment-2700065495
https://github.com/ItsJokerZz/FPKGi/issues/21#issuecomment-3005424499

O sistema continua a receber um link remoto. Não foi introduzido um proxy
local que dependa de manter esta aplicação aberta. Verificação de PKG,
HTTPS, HTTP Range, proteção de aplicações já instaladas, código de quatro
números e torrents em /data/pkg continuam presentes.

Verificação: compilação OpenOrbis sem erros; 53 testes de URL/cabeçalho/Range,
43 da fila PS4, 16 do acesso temporário, 21 do resolvedor/auth e testes web.
Os novos testes comparam o pedido HTTP derivado de URLs originais e preparadas,
incluindo parâmetros assinados, e verificam que o cabeçalho é pedido com o
endereço original. Limites de tamanho e falhas antes do registo são cobertos.

O PKG final tem APP_VER 00.32 e TITLE_ID HBRW00001. Instalar por cima da
0.1.21 e enviar um link original ainda válido. Confirmar em Notificações >
Transferências que aparecem bytes descarregados antes de testar o repouso.
Ainda requer confirmação na PS4 real; os testes simulam os serviços.
Versão experimental local, sem publicação no GitHub.

Teste posterior na consola (01/10/2026): o utilizador confirmou que o
download estava a decorrer apesar da indicação «A calcular». A alteração
adicional de registo preparada durante o diagnóstico foi retirada; mantém-se
a implementação 0.1.22. Instalação concluída e modo repouso ainda não confirmados.
