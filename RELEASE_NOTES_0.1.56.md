# h1pNoise 0.1.56 — apagar updates antigas

Novo botão **Apagar updates antigas** em **Atualizações da app**, na página do telemóvel. Escolhe disco interno (/data/pkg), Pen USB 1 ou Pen USB 2 e confirma em **Apagar**. Cancelar não apaga nada.

A limpeza só procura os nomes exatos `h1pNoise-update-<build>.pkg` e `h1pNoise-update-<build>.pkg.part`, diretamente no destino escolhido e de builds anteriores à versão instalada. Preserva a versão instalada, todas as mais recentes, os outros PKG, os torrents, os registos e as subpastas. Não limpa pastas recursivamente nem desinstala aplicações.

É bloqueada durante uma transferência ou atualização. Mantém os dados de qualquer atualização já descarregada e apresenta o resultado da limpeza separadamente. Se houver uma falha de acesso, para e informa quantos ficheiros foram apagados.

## Instalar e testar

1. Na versão 0.1.55, procura e descarrega a 0.1.56, ou usa o PKG desta release.
2. Fecha a h1pNoise e instala pelo Package Installer do GoldHEN, com **Enable Background Installation** desligado.
3. Abre a 0.1.56 e atualiza a página do telemóvel para aparecer o botão.
4. Escolhe **Apagar updates antigas**, o destino e **Apagar**. Liga a pen diretamente à PS4 para testar USB.

As atualizações continuam a instalar-se manualmente. Mantém a correção de escrita na raiz USB da 0.1.55.

## Validação

212 verificações do atualizador no PC e testes da interface. Inclui preservação de PKG alheios à atualização, ficheiros instalados/mais recentes, subpastas, estado de atualização e cópias noutros destinos; confirmação/cancelamento, destinos inválidos, pen ausente e falhas parciais de acesso. Os testes das chamadas PS4 são simulados; é necessário confirmar o comportamento na consola real.
