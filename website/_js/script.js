// ===== Configuração =====
const ESP_IP = "192.168.4.1";          // IP padrão do ESP32 em modo Access Point
const URL_BASE = `http://${ESP_IP}`;
const INTERVALO_ENVIO = 60;             // ms mínimos entre comandos do mesmo dedo

const sliders = [...document.querySelectorAll(".slider")];
const botoesDedo = [...document.querySelectorAll(".fnd-botao button[data-dedo]")];
const ultimoEnvio = new Array(sliders.length).fill(0);

// converte o valor do slider (0 a 100) em ângulo do servo (0 a 180)
function paraAngulo(slider) {
    const min = Number(slider.min || 0);
    const max = Number(slider.max || 100);
    return Math.round(((Number(slider.value) - min) / (max - min)) * 180);
}

async function enviarAngulo(id, angulo, forcar = false) {
    const agora = Date.now();
    if (!forcar && agora - ultimoEnvio[id] < INTERVALO_ENVIO) return;
    ultimoEnvio[id] = agora;

    const controle = new AbortController();
    const t = setTimeout(() => controle.abort(), 1500);
    try {
        await fetch(`${URL_BASE}/servo?id=${id}&angulo=${angulo}`, { signal: controle.signal });
    } catch (e) {
        console.warn("Sem conexão com o ESP32 (" + ESP_IP + ")");
    } finally {
        clearTimeout(t);
    }
}

// ===== Sliders =====
sliders.forEach((slider, i) => {
    slider.addEventListener("input", () => enviarAngulo(i, paraAngulo(slider)));
    // garante que o valor final chegue ao ESP
    slider.addEventListener("change", () => enviarAngulo(i, paraAngulo(slider), true));
});

// ===== Botões inferiores: abre/fecha o dedo correspondente =====
botoesDedo.forEach((botao) => {
    const i = Number(botao.dataset.dedo);
    botao.addEventListener("click", () => {
        const max = Number(sliders[i].max || 100);
        const min = Number(sliders[i].min || 0);
        const aberto = Number(sliders[i].value) > (min + max) / 2;
        sliders[i].value = aberto ? min : max;
        enviarAngulo(i, paraAngulo(sliders[i]), true);
    });
});
