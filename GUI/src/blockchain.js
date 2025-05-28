const fs = require("fs")
const crypto = require("crypto")
const grpc = require('@grpc/grpc-js');
const { connect, signers, hash } = require('@hyperledger/fabric-gateway');
const Chart = require('chart.js/auto');

let tempChart, humidChart, pressChart, accelChart, tvocChart, rgbChart;

const certPem = fs.readFileSync('./keys/cert.pem');
const keyPem = fs.readFileSync('./keys/key.pem');
const tlsRoot = fs.readFileSync('./keys/ca.crt');

const identity = { mspId: 'Org1MSP', credentials: certPem };
const signer = signers.newPrivateKeySigner(
    crypto.createPrivateKey(keyPem));

const peerEndpoint = 'peer0.org1.example.com:7051';

const dropdown = document.getElementById('dynamic-dropdown');
const refresh = document.getElementById('refresh');

dropdown.addEventListener('change', (event) => {

    dataPoints = [];
    const selectedValues = Array.from(dropdown.selectedOptions)
        .map(option => option.value);
    // console.log(selectedValues);
    selectedValues.forEach(uuid => {
        // console.log(uuid);
        getData(uuid);
    });
});

refresh.addEventListener('click', (event) => {
    const selectedValues = Array.from(dropdown.selectedOptions)
        .map(option => option.value);
    selectedValues.forEach(uuid => {
        getData(uuid);
    });
});


async function getData(uuid) {
    const client = new grpc.Client(
        peerEndpoint,
        grpc.credentials.createSsl(tlsRoot)
    );

    const gateway = connect({
        identity,
        signer,
        hash: hash.sha256,
        client,
    });

    try {
        const network = gateway.getNetwork('sensordata');
        const contract = network.getContract('sensorCC');

        const resultBytes = await contract.evaluateTransaction('QueryDevice', uuid);
        let jsonStr = resultBytes.toString();

        if (/^\d+,/.test(jsonStr)) {
            const byteArray = Uint8Array.from(
                jsonStr.split(',').map(n => parseInt(n, 10)));
            jsonStr = Buffer.from(byteArray).toString('utf8');
        }

        const readings = JSON.parse(jsonStr);
        console.dir(readings, { depth: null });
        let temperatures = readings.map(r => Number(r.temperature));
        let humidities = readings.map(r => Number(r.humidity));
        let pressures = readings.map(r => Number(r.pressure));
        let tvocs = readings.map(r => Number(r.tvoc));
        let x = readings.map(r => Number(r.accel_x));
        let y = readings.map(r => Number(r.accel_y));
        let z = readings.map(r => Number(r.accel_z));
        let r = readings.map(r => Number(r.r));
        let g = readings.map(r => Number(r.g));
        let b = readings.map(r => Number(r.b));

        addData(tempChart, temperatures, 0);
        addData(humidChart, humidities, 0);
        addData(pressChart, pressures, 0);
        addData(accelChart, x, 0);
        addData(accelChart, y, 1);
        addData(accelChart, z, 2);
        addData(tvocChart, tvocs, 0);
        addData(rgbChart, r, 0);
        addData(rgbChart, g, 1);
        addData(rgbChart, b, 2);

        let timestamps = readings.map(r => (new Date(Number(r.timestamp))).toLocaleString());
        addLabel(timestamps);
    } finally {
        gateway.close();
        client.close();
    }
}

function addLabel(labels) {

    tempChart.data.labels = labels;
    humidChart.data.labels = labels;
    pressChart.data.labels = labels;
    accelChart.data.labels = labels;
    tvocChart.data.labels = labels;
    rgbChart.data.labels = labels;

    tempChart.data.labels = tempChart.data.labels.slice(-60);
    humidChart.data.labels = humidChart.data.labels.slice(-60);
    pressChart.data.labels = pressChart.data.labels.slice(-60);
    accelChart.data.labels = accelChart.data.labels.slice(-60);
    tvocChart.data.labels = accelChart.data.labels.slice(-60);
    rgbChart.data.labels = accelChart.data.labels.slice(-60);

    tempChart.update('none');
    humidChart.update('none');
    pressChart.update('none');
    accelChart.update('none');
    tvocChart.update('none');
    rgbChart.update('none');
}

function addData(chart, newData, index) {
    chart.data.labels.filter((value, index, array) => { return array.indexOf(value) === index; });
    chart.data.datasets[index].data = newData;
    chart.data.datasets[index].data = chart.data.datasets[index].data.slice(-60);
}

(async function () {

    tempChart = new Chart(
        document.getElementById('temp'),
        {
            type: 'line',
            data: {
                labels: [],
                datasets: [
                    {
                        label: 'Temperature',
                        data: []
                    }
                ]
            },
            options: {
                scales: {
                    y: {
                        suggestedMin: 10,
                        suggestedMax: 40,
                        ticks: {
                            callback: function (value) {
                                return value + '°C';
                            }
                        }
                    },

                },

            }
        }
    );
    humidChart = new Chart(
        document.getElementById('humid'),
        {
            type: 'line',
            data: {
                labels: [],
                datasets: [
                    {
                        label: 'Humidity',
                        data: []
                    }
                ]
            },
            options: {
                scales: {
                    y: {
                        suggestedMin: 50,
                        suggestedMax: 100,
                        ticks: {
                            callback: function (value) {
                                return value + '%';
                            }
                        }
                    }
                },
            }
        }
    );

    pressChart = new Chart(
        document.getElementById('press'),
        {
            type: 'line',
            data: {
                labels: [],
                datasets: [
                    {
                        label: 'Pressure',
                        data: []
                    }
                ]
            },
            options: {
                scales: {
                    y: {
                        suggestedMin: 90,
                        suggestedMax: 110,
                        ticks: {
                            callback: function (value) {
                                return value + 'kPa';
                            }
                        }
                    }
                },
            }
        }
    );

    accelChart = new Chart(
        document.getElementById('accel'),
        {
            type: 'line',
            data: {
                labels: [],
                datasets: [
                    {
                        label: 'Acceleration X',
                        data: []
                    },
                    {
                        label: 'Acceleration Y',
                        data: []
                    },
                    {
                        label: 'Acceleration Z',
                        data: []
                    }
                ]
            },
            options: {
                scales: {
                    y: {
                        suggestedMin: -10,
                        suggestedMax: 10,
                        ticks: {
                            callback: function (value) {
                                return value + 'm/s^2';
                            }
                        }
                    }
                },
            }
        }
    );

    tvocChart = new Chart(
        document.getElementById('tvoc'),
        {
            type: 'line',
            data: {
                labels: [],
                datasets: [
                    {
                        label: 'TVOC',
                        data: []
                    }
                ]
            },
            options: {
                scales: {
                    y: {
                        suggestedMin: 0,
                        suggestedMax: 500,
                        ticks: {
                            callback: function (value) {
                                return value + 'ppb';
                            }
                        }
                    }
                },
            }
        }
    );

    rgbChart = new Chart(
        document.getElementById('rgb'),
        {
            type: 'line',
            data: {
                labels: [],
                datasets: [
                    {
                        label: 'Red',
                        borderColor: 'rgb(255,  40,  40)',
                        backgroundColor: 'rgba(255,  40,  40, .15)',
                        data: []
                    },
                    {
                        label: 'Green',
                        borderColor: 'rgb( 40, 200,  40)',
                        backgroundColor: 'rgba( 40, 200,  40, .15)',
                        data: []
                    },
                    {
                        label: 'Blue',
                        borderColor: 'rgb( 40, 120, 255)',
                        backgroundColor: 'rgba( 40, 120, 255, .15)',
                        data: []
                    }
                ]
            },
            options: {
                scales: {
                    y: {
                        suggestedMin: 0,
                        suggestedMax: 255,
                        ticks: {
                            callback: function (value) {
                                return value + 'lx';
                            }
                        }
                    }
                },
            }
        }
    );
})();

const intervalId = setInterval(async () => {
  try {
    const selectedValues = Array.from(dropdown.selectedOptions)
        .map(option => option.value);
    console.log("Running");
    selectedValues.forEach(async uuid => {
        // console.log(uuid);
        await getData(uuid);
    });
  } catch (err) {
    console.error('Task failed:', err);
  }
}, 5000);