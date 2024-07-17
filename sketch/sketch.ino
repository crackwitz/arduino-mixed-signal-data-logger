// this will read all ADC channels and print voltages to serial
// this will run on an Arduino Nano

// Arduino Nanos with old bootloader, put this in arduino.json:
// "configuration": "cpu=atmega328old",
// "board": "arduino:avr:nano",

#include <Arduino.h>


uint32_t sched = 0;
uint32_t sampling_period = 1000;
bool sampling_enabled = true;
bool print_timestamp = false;

int const num_analog_channels = 8;
int const num_digital_channels = 20;
bool enabled_analog[num_analog_channels] { false }; // A0 .. 7
bool enabled_digital[num_digital_channels] { false }; // D0 .. 19
// D14..19 = A0..5
// int const analog_pins[8] { PIN_A0, PIN_A1, PIN_A2, PIN_A3, PIN_A4, PIN_A5, PIN_A6, PIN_A7 };


/**
 * @brief Keep to a schedule. Return true when due.
 *
 * Increments sched. Catches up if late.
 *
 * @param pnow[out] current time
 * @param sched[inout] next scheduled execution
 * @param sampling_period period in ms
 * @return true Execution is due.
 * @return false Execution is not due.
 */
bool doPeriodically(uint32_t *pnow, uint32_t *sched, uint32_t sampling_period)
{
    uint32_t const now = millis();
    int32_t const diff = (now - *sched);

    if (diff < 0)
        return false;

    *pnow = now;

    *sched += sampling_period;

    if ((int32_t)(now - *sched) > 0) // late, catch up
    {
        *sched = now;
        Serial.println(F("# WARNING: missed a sample"));
    }

    return true;
}


void print_usage()
{
    Serial.println(F("# Data logger for Arduino Nano"));
    Serial.println(F("# Authored by Christoph Rackwitz, 2024-06-14"));
    Serial.println(F("# This periodically reads analog and digital inputs, writes to UART"));
    Serial.println(F("# Analog channel values are in range 0..1023."));
    Serial.println(F("# Digital channel values are reported as 0 or 1024."));
    Serial.println(F("# Backspace is supported for line editing. Ctrl-C discards all input."));
    Serial.println(F("# Usage: <command><ENTER>"));
    Serial.println(F("#   h help ?    print help"));
    Serial.println(F("#   s           print status (enabled channels, sampling period/freq)"));
    Serial.println(F("#   b<baud>     change baud rate. 1000000 and 2000000 baud are supported."));
    Serial.println(F("#   <ENTER>     toggle sampling (blank command)"));
    Serial.println(F("#   <CTRL-C>    stop sampling"));
    Serial.println(F("#   .           stop sampling"));
    Serial.println(F("#   p<period>   set sampling period in ms"));
    Serial.println(F("#   f<freq>     set sampling frequency in Hz, floats allowed, 1 ms resolution"));
    Serial.println(F("#   t           toggle printing of timestamp (in ms)"));
    Serial.println(F("#   aN,N...  a  disable analog channels, N = 0..7"));
    Serial.println(F("#   AN,N...  A  enable analog channels,  N = 0..7"));
    Serial.println(F("#   dN,N...  d  disable digital channels, N = 0..19"));
    Serial.println(F("#   DN,N...     enable digital channels,  N = 0..19"));
}


void print_status()
{
    Serial.print(F("# Sampling: "));
    Serial.print(sampling_enabled ? F("enabled, ") : F("disabled, "));
    Serial.print(sampling_period);
    Serial.print(F(" ms ("));
    Serial.print(1000.f / sampling_period);
    Serial.print(F(" Hz), "));
    Serial.print(print_timestamp ? F("with") : F("without"));
    Serial.println(F(" timestamps"));

    Serial.print(F("# Analog:   "));
    for (int index = 0; index < num_analog_channels; ++index)
    {
        // Serial.print(enabled_analog[index] ? '+' : '-');
        // Serial.print(index); Serial.print(' ');

        if (enabled_analog[index])
        {
            if (index < 10) Serial.print(' '); Serial.print(index); Serial.print(' ');
        }
        else
            Serial.print(F(" _ "));

    }
    Serial.println(F("(A0..7)"));
    // Serial.println();

    Serial.print(F("# Digital:  "));
    for (int index = 0; index < num_digital_channels; ++index)
    {
        // Serial.print(enabled_digital[index] ? '+' : '-');
        // Serial.print(index); Serial.print(' ');

        if (enabled_digital[index])
        {
            if (index < 10) Serial.print(' '); Serial.print(index); Serial.print(' ');
        }
        else
            Serial.print(F(" _ "));
    }
    Serial.println(F("(D0..19, D14..19 = A0..5)"));
}


enum channel_type_t { CT_ANALOG = 1, CT_DIGITAL = 2 };

enum action_type_t { AT_DISABLE = 0, AT_ENABLE = 1 };

// this is required. Arduino seems to prepend its own idea of prototypes during build.
void set_channels(channel_type_t ctype, int start, int stop, action_type_t action);

void set_channels(channel_type_t ctype, int start, int stop, action_type_t action)
{
    for (int index = start; index <= stop; ++index)
    {
        if (ctype == CT_ANALOG)
            enabled_analog[index] = (action == AT_ENABLE);
        else if (ctype == CT_DIGITAL)
            enabled_digital[index] = (action == AT_ENABLE);
    }
}


void handle_channel_toggle(String line)
{
    char const action_char = line[0];

    channel_type_t const ctype =
        (action_char == 'A' || action_char == 'a') ? CT_ANALOG :
        (action_char == 'D' || action_char == 'd') ? CT_DIGITAL :
        0;

    action_type_t const action =
        (action_char == 'A' || action_char == 'D') ? AT_ENABLE :
        (action_char == 'a' || action_char == 'd') ? AT_DISABLE :
        0;

    int const valid_range =
        (ctype == CT_ANALOG) ? num_analog_channels :
        (ctype == CT_DIGITAL) ? num_digital_channels :
        0;

    String const argument = line.substring(1);
    int const arglen = argument.length();

    if (arglen == 0) // all
    {
        if (ctype == CT_DIGITAL && action == AT_ENABLE)
        {
            Serial.println(F("# ERROR: will NOT enable all digital channels"));
        }
        else
        {
            set_channels(ctype, 0, valid_range - 1, action);
        }
    }
    else
    {
        int start = 0;
        for (int i = 0; i <= arglen; ++i)
        {
            if (i == arglen || argument[i] == ',')
            {
                String const substr = argument.substring(start, i);
                int const colon = substr.indexOf(':');
                if (colon != -1)
                {
                    int const start = substr.substring(0, colon).toInt();
                    int const stop = substr.substring(colon + 1).toInt();
                    set_channels(ctype, start, stop, action);
                }
                else
                {
                    int const index = substr.toInt();
                    set_channels(ctype, index, index, action);
                }
                start = i + 1;
            }
        }
    }
}


/**
 * @brief Parse commands from serial
 * See usage above.
 */
void handle_serial_input()
{
    static char line_break_char = 0;
    static String linebuf = "";
    bool line_complete = false;

    while (Serial.available() > 0)
    {
        char const c = Serial.read();

        if (c == '\r' || c == '\n')
        {
            if (!line_break_char)
                line_break_char = c;

            if (c == line_break_char)
            {
                line_complete = true;
                break;
            }
            else
            {
                continue; // ignore the other of CR/LF
            }
        }
        else
        {
            line_break_char = 0;
        }

        if (c == 0x08 || c == 127) // backspace
        {
            if (linebuf.length() > 0)
                linebuf.remove(linebuf.length() - 1);
        }
        else if (c == 0x03) // Ctrl+C
        {
            Serial.println(F("# ABORT"));
            // rewrite as "stop" command
            linebuf = F("."); line_complete = true;
            break;
        }
        // else if (c == 27) // initiates escape sequence, followed by 91 '[' and then something else (perhaps a letter)
        else
        {
            if (c >= 32 && c <= 126)
                linebuf += c;
            else if (c == 27)
                linebuf += '^';
        }
    }

    if (!line_complete)
        return;

    bool do_print_status = false;

    if (linebuf == F("h") || linebuf == F("?") || linebuf == F("help"))
    {
        print_usage();
    }

    else if (linebuf == F("s")) // status
    {
        do_print_status = true;
    }

    else if (linebuf == F("."))
    {
        sampling_enabled = false;
        do_print_status = true;
    }
    else if (linebuf == F(""))
    {
        sampling_enabled = !sampling_enabled;
        if (sampling_enabled)
            sched = millis() + sampling_period;
        do_print_status = true;
    }

    else if (linebuf.startsWith(F("b"))) // baud rate
    {
        if (linebuf.length() <= 1)
        {
            Serial.println(F("# ERROR: missing baud rate value after command 'b'"));
        }
        else
        {
            long int const baud = linebuf.substring(1).toInt();
            Serial.print(F("# Changing baud rate to: ")); Serial.println(baud);
            Serial.begin(baud);
            Serial.print(F("# Baud rate changed to: ")); Serial.println(baud);
        }
    }

    else if (linebuf.startsWith(F("p"))) // period
    {
        if (linebuf.length() <= 1)
        {
            Serial.println(F("# ERROR: missing period value after command 'p'"));
        }
        else
        {
            sampling_period = linebuf.substring(1).toInt();
            sched = millis() + sampling_period;
            do_print_status = true;
        }
    }
    else if (linebuf.startsWith(F("f"))) // frequency
    {
        if (linebuf.length() <= 1)
        {
            Serial.println(F("# ERROR: missing frequency value after command 'f'"));
        }
        else
        {
            float const freq = linebuf.substring(1).toFloat();
            float const new_period = 1000.f / freq;
            sampling_period = roundf(new_period);
            sched = millis() + sampling_period;
            do_print_status = true;
        }
    }
    else if (linebuf == F("t")) // timestamp
    {
        print_timestamp = !print_timestamp;
        do_print_status = true;
    }

    else if (linebuf.startsWith(F("A")) || linebuf.startsWith(F("a")) || linebuf.startsWith(F("D")) || linebuf.startsWith(F("d")))
    {
        handle_channel_toggle(linebuf);
        do_print_status = true;
    }

    else
    {
        Serial.print(F("# ERROR: unknown command: "));
        Serial.println(linebuf);
        print_usage();
    }


    if (do_print_status)
        print_status();

    linebuf = "";
}


void setup()
{
    // set all ADC channels to input
    pinMode(PIN_A0, INPUT);
    pinMode(PIN_A1, INPUT);
    pinMode(PIN_A2, INPUT);
    pinMode(PIN_A3, INPUT);
    pinMode(PIN_A4, INPUT);
    pinMode(PIN_A5, INPUT);
    pinMode(PIN_A6, INPUT);
    pinMode(PIN_A7, INPUT);

    // https://arduino.stackexchange.com/questions/296/how-high-of-a-baud-rate-can-i-go-without-errors
    Serial.begin(115200);
    // Serial.begin(125000);
    // Serial.begin(250000);
    // Serial.begin(500000);
    // Serial.begin(1000000);
    // Serial.begin(2000000);

    print_usage();
    print_status();
}


void loop()
{
    handle_serial_input();

    if (!sampling_enabled)
        return;

    uint32_t now = 0;
    if (!doPeriodically(&now, &sched, sampling_period))
        return;

    bool first_written = false;

    if (print_timestamp)
    {
        Serial.print(now);
        first_written = true;
    }

    for (int index = 0; index < num_analog_channels; ++index)
    {
        if (enabled_analog[index])
        {
            uint16_t const val = analogRead(index);
            if (first_written) Serial.print(',');
            Serial.print(val); first_written = true;
        }
    }

    for (int index = 0; index < num_digital_channels; ++index)
    {
        if (enabled_digital[index])
        {
            bool const val = digitalRead(index);
            if (first_written) Serial.print(',');
            Serial.print(val ? "1024" : "0"); first_written = true;
        }
    }

    Serial.println();
}
