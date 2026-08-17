# AGENTS.md — PatiqVulkanEngine

Guia direto para qualquer agente (ou humano) trabalhando neste repo.

## Build & Run

```bash
make          # compila (shaders + objetos), gera build/first_app.out
make test     # compila e roda o jogo
make clean    # remove build/ e shaders/compiled/
```

- `make test` **trava o terminal** enquanto a janela está aberta. Só retorna com `Ctrl+C` no terminal ou fechando a janela do app. Um agente nunca deve rodar `make test` esperando retorno imediato — ou roda em background com timeout, ou avisa o usuário que precisa fechar a janela manualmente.
- Só `make` (sem `test`) é o jeito certo de validar que o código compila sem travar a sessão.
- Erros de link geralmente são libs faltando no `LDFLAGS` do Makefile, não bug de código.
- Shaders `.vert`/`.frag` são compilados automaticamente pro `.spv` via `GLSLC_PATH` (do `.env`). Se mexer em shader, `make` já recompila o necessário.

## Metodologia caveman

Regra central deste projeto: **fazer a coisa mais simples que funciona, ver funcionando, só então complicar.**

- **Uma mudança por vez.** Implementa uma coisa pequena, roda `make`, confirma que compilou, só depois segue pra próxima. Nunca empilhar 5 features sem compilar entre elas.
- **Sem abstração antecipada.** Não criar interfaces, factories, templates genéricos ou "flexibilidade pra o futuro" que ninguém pediu. Copiar-e-adaptar um padrão que já existe no código (ex: como `SimpleRenderSystem` e `PointLightSystem` são estruturados) é preferível a inventar uma abstração nova.
- **printf/cout debugging é válido.** Não tem tempo pra configurar debugger gráfico pra Vulkan. `std::cerr` com contexto (nome da função, valores) resolve 90% dos casos. Vulkan validation layers (já habilitadas via `vulkan-validationlayers`) fazem o resto.
- **Compilar cedo, compilar sempre.** Cada `.cpp`/`.hpp` tocado deve compilar antes de seguir pro próximo arquivo. Erros de template/linker em Vulkan são dolorosos de rastrear se acumularem.
- **Se travou mais de ~15-20 min num bug, simplificar o problema.** Comentar código, isolar o menor caso que reproduz o crash/validation error, resolver isolado, depois reintegrar.

## Boas práticas Vulkan usadas/esperadas neste código

- **RAII em tudo que é recurso Vulkan.** Classes wrapper (`PveBuffer`, `PveDescriptorPool`, `PveDescriptorSetLayout`) devem sempre destruir o handle correspondente no destrutor. Nunca segurar `VkBuffer`/`VkImage`/`VkDescriptorSet` cru fora de um wrapper RAII.
- **Descriptor sets: um layout, N sets (um por frame in flight).** Ver o padrão em `run()` — `globalSetLayout` é criado uma vez, `globalDescriptorSets` tem `MAX_FRAMES_IN_FLIGHT` entradas. Não reusar o mesmo descriptor set entre frames em voo.
- **`vkDeviceWaitIdle` só no shutdown**, nunca dentro do loop de render — mata performance.
- **Uma única `Builder` fluente por objeto Vulkan complexo** (descriptor pool, descriptor set layout, pipeline). Seguir esse padrão já estabelecido em vez de construir structs `VkXxxCreateInfo` na mão espalhado pelo código.
- **Ordem de render importa e deve ficar comentada.** Ex: `simpleRenderSystem.renderGameObjects()` antes de `pointLightSystem.render()` — isso é intencional (transparência/blending), não trocar sem entender por quê.
- **Clamp de frame time** (`MAX_FRAME_TIME`) sempre presente pra evitar "spiral of death" em physics/animação quando o frame demora muito (breakpoint, resize de janela etc).
- **Nunca alocar Vulkan objects dentro do loop principal.** Buffers, descriptor sets, pipelines: todos criados antes do `while(!shouldClose())`.

## Boas práticas C++ usadas/esperadas

- `std::unique_ptr` pra ownership único de sistemas (`shadowMapSystem`, `uiRenderSystem`, `viewerObject`). `std::shared_ptr` só quando há ownership realmente compartilhado (ex: `PveModel` entre múltiplos `PveGameObject`).
- `static bool xPressed` dentro de `handleInput` é o padrão do projeto pra debounce de tecla/mouse — seguir esse mesmo padrão pra qualquer novo input, não introduzir uma lib de input handling.
- Structs de dados simples (`UIButton`, `FrameInfo`, `GlobalUbo`) sem métodos, só campos — não transformar em classes com getters/setters.
- Namespacing: tudo dentro de `namespace pve {}`. Não vazar símbolos globais.
- `#define GLM_FORCE_RADIANS` e `#define GLM_FORCE_DEPTH_ZERO_TO_ONE` são obrigatórios em qualquer novo arquivo que inclua `glm` diretamente e mexa com projeção/rotação — inconsistência aqui causa bugs silenciosos de câmera.
- Evitar `using namespace std;` — o código já não usa, manter assim.
- i18n: qualquer string visível na UI passa por `i18n.get("chave")`, nunca string literal hardcoded na tela (comparar com `hackerGreen` etc., que são cores, não texto).

## Convenções de nomes já em uso (seguir sem inventar novo padrão)

- Classes: `PascalCase` prefixado com `Pve` para core engine (`PveBuffer`, `PveCamera`, `PveDevice`) e sem prefixo para systems/controllers (`SimpleRenderSystem`, `KeyboardMovementController`).
- Membros e variáveis locais: `camelCase`.
- Arquivos: `snake_case.hpp` / `snake_case.cpp`, agrupados por pasta (`pve/`, `systems/`, `controllers/`).

## Checklist antes de considerar uma tarefa pronta

1. `make` compila sem warning novo.
2. Se mexeu em shader: `.spv` foi regerado (o Makefile cuida disso automaticamente).
3. Se mexeu em Vulkan resource: destrutor correspondente existe e é chamado.
4. Testou manualmente com `make test` (avisando que trava o terminal até fechar a janela ou `Ctrl+C`).