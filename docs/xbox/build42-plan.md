# Build 42 — requisitos e plano de implementação

## Objetivo

Evoluir a Build 41 do Jadyx Xbox UWP sem remover recursos existentes, priorizando estabilidade, desempenho do Super Mario Bros. Wonder e compatibilidade com os demais jogos. Nenhuma build deve ser recomendada para instalação sobre a versão existente até validar o pacote, o identificador de aplicação e o comportamento dos dados locais.

## Requisitos obrigatórios

1. **Atualizações de jogos** — identificar uma atualização colocada junto do jogo base, verificar a correspondência do título, não substituir nem apagar o arquivo base e evitar importações duplicadas. Validar com arquivos de teste legais antes de declarar concluído.
2. **Desempenho e RAM** — medir gargalos antes de otimizar. Não tentar alocar 100% da RAM: o sistema, o frontend, o emulador e a GPU também precisam de memória. Preferir limites explícitos, tratamento de pressão de memória, liberação segura de caches e prevenção de falhas por falta de memória.
3. **Super Mario Bros. Wonder** — criar procedimento reproduzível de teste em cenas iguais, registrando FPS/frametime quando disponível, travamentos, artefatos, consumo de memória e estabilidade após reiniciar. Não prometer 100% liso sem medições no Xbox Series S.
4. **Perfis por jogo** — permitir preferências independentes para cada jogo, sem alterar inadvertidamente os demais. Preservar valores desconhecidos de versões anteriores sempre que possível.
5. **Gráficos** — expor somente opções conectadas a configurações reais do backend. Priorizar resolução interna/escala de renderização e upscaling já suportados. Texturas, sombras e Bloom só podem ser alterados quando o jogo ou o backend oferecer suporte; não apresentar controles falsos.
6. **Limite de quadros** — oferecer apenas limites implementados e suportados (por exemplo, 30/60 quando aplicável). 120/240/ilimitado não garantem essa taxa; considerar sincronização vertical, limite do jogo e desempenho real.
7. **Menu durante o jogo** — implementar um atalho de controle que não conflite com comandos dos jogos; testar toque breve, pressão longa e controle desconectado. O botão Menu do sistema pode ser reservado pelo Xbox, então a alternativa deve ser documentada se não puder ser capturado.
8. **Português brasileiro** — adicionar catálogo pt-BR completo, traduzindo todos os IDs da interface, preservando placeholders como {0}/{1} e regras de plural. Registrar idioma no seletor e verificar compilação que rejeita traduções ausentes ou duplicadas.
9. **Assistente de configuração** — recomendar perfis a partir de métricas e resultados conhecidos, com opção manual, explicação da alteração e possibilidade de reverter. Não alegar IA capaz de corrigir automaticamente toda incompatibilidade.
10. **Diagnóstico** — registrar de forma útil falhas de carregamento, tela preta, falha gráfica, áudio sem imagem e pressão de memória, quando os sinais necessários estiverem disponíveis.
11. **Segurança de dados** — confirmar Package Family Name/identidade de pacote, diretórios LocalState/RoamingState e comportamento de atualização. Criar backup/exportação de saves, configurações e metadados quando tecnicamente possível; nunca apagar biblioteca como etapa de atualização. Testar migração com cópia de dados, não com o único save do usuário.
12. **Fontes de jogos** — apontar somente jogos homebrew e títulos gratuitos/distribuídos oficialmente; não integrar busca ou download de cópias comerciais não autorizadas.

## Plano de trabalho e critérios de aceitação

### Fase A — baseline e compilação
- Registrar commit-base e manter a Build 41 intacta.
- Compilar pacote UWP e guardar logs/artifacts.
- Verificar avisos e erros antes de avançar.

### Fase B — importação de updates
- Revisar a implementação de detecção de atualizações adjacentes.
- Testar jogo sem update, update correspondente, update de título diferente, arquivo inválido e importação repetida.
- Critério: base preservada, nenhum update incompatível associado e nenhuma duplicação.

### Fase C — estabilidade e memória
- Auditar caminhos de alocação/cache existentes antes de alterar limites.
- Evitar limpar caches em uso ou aumentar alocações sem limite.
- Critério: sem regressões de inicialização, sem encerramento por memória nos testes disponíveis e logs diagnósticos suficientes.

### Fase D — opções de desempenho por jogo
- Reutilizar opções já conectadas ao backend; não inventar controles de GPU sem implementação.
- Salvar e carregar perfis independentes e permitir restaurar padrão.
- Critério: mudança persistente no jogo correto, sem alterar outros perfis.

### Fase E — pt-BR e menu no jogo
- Traduzir catálogo inteiro e registrar idioma.
- Testar todos os menus, placeholders e textos de erro.
- Testar atalho com controles suportados e evitar captura de comandos comuns.

### Fase F — proteção de dados e entrega
- Testar atualização em ambiente com cópia dos dados.
- Confirmar identidade de pacote e comportamento de instalação.
- Publicar notas com funções concluídas, limitações e instruções de backup.
- Não recomendar instalação sobre uma build existente antes de cumprir os critérios.

## Estado inicial conhecido nesta branch

- A branch `build-42-graphics-updates-ptbr` parte da base de Build 41.
- Foi copiado `src/jadyx/content/content_service.cpp` com lógica candidata para importação de updates adjacentes; requer revisão e testes.
- O frontend existente possui controlador de resolução adaptativa e opções de upscaling FSR/bilinear.
- O catálogo de idiomas existente contém inglês e espanhol; pt-BR ainda precisa ser implementado.
- Os controles de FPS, textura, sombras e Bloom não devem ser considerados concluídos até encontrar e testar a ligação real com o backend.
- O workflow de compilação deve ser considerado pendente até terminar e produzir um artefato válido.

## Regra de comunicação

Distinguir sempre: **implementado no código**, **compilado**, **testado no Xbox** e **confirmado como seguro para atualizar**. Esses estados são diferentes.
