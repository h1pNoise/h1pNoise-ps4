# h1pNoise 0.1.19 — teste local PS4

Corrige uma validação introduzida em 0.1.18 que rejeitava o objeto prison0
quando colocado na região de dados estáticos do kernel. O código original
da libjbc já aceita essa região nas operações de referência do objeto.
As verificações de processos, descritores e vnodes continuam restritas
à região heap. A validação usada ao aplicar credenciais foi corrigida
da mesma forma.

O registo enviado da consola confirma que jbc_get_cred completou e que
jbc_jailbreak_cred falhou, antes da alteração de credenciais ou carregamento
de AppInstUtil. A 0.1.18 não registava a etapa interna, pelo que não permite
confirmar que esta validação foi a causa concreta na PS4 do utilizador.
A 0.1.19 regista essa etapa se a resolução continuar a falhar, sem guardar
endereços do kernel, URLs ou tokens.

Verificação: compilação OpenOrbis; 42 testes de PKG, 25 da fila de instalação,
9 de duração do acesso temporário, 16 da implementação real do resolvedor
sobre um mapa de memória artificial, e testes da página web.
Os 16 testes incluem prison0 estático ou heap, leitura/aplicação/reposição
dos campos de credenciais, endereços inválidos, falhas de leitura, lista
alterada ou terminada, identidade do processo e indisponibilidade do kernel.
Estes testes não executam syscalls no kernel da PS4, a gestão real de
referências de vnodes nem a instalação em firmware 13.50.

Mantém o código de quatro números e os torrents em /data/pkg.
Esta versão experimental não foi publicada no GitHub.
