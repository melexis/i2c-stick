const mlx_steel_grey = "#b2c4cb";
const mlx_electric_green = "#65BBA9";
const mlx_gold_yellow = "#EEA320";
const mlx_coral_red = "#DB4140";
const mlx_royal_blue = "#00354B";
const mlx_charcoal_grey = "#3e4242";


const color_palette = [mlx_electric_green, mlx_coral_red, mlx_gold_yellow, mlx_royal_blue, mlx_steel_grey, "#e60049", "#0bb4ff", "#50e991", "#e6d800", "#9b19f5", "#ffa300", "#dc0ab4", "#b3d4ff", "#00bfa0"];

var has_serial = false;
var serial = null;
var accepted_ports = [{usbProductId: 33033, usbVendorId: 9114},
                      {usbProductId: 33033, usbVendorId: 11914}
                     ];
var selected_port = accepted_ports[1];

var keep_reading = true;
var reader;
var writer;
var closed_promise;
var enc = new TextEncoder(); // always utf-8
var dec = new TextDecoder("utf-8");
var transient_chart = null;
var receive_buffer = "";
var t_min = 15;
var t_max = 35;
var spatial_previous_orientation = 0;

var connected_slaves = {};
var heat_map_gradient_colors = null;


function set_terminal_height()
{
  setTimeout(function() {
    let total_height = $(window).height();
    let height1 = $("#serial_send").outerHeight();
    let height2 = $("nav.navbar").outerHeight();
    let height3 = $("div#tab_interface > div.tabs").outerHeight();
    let new_height = total_height - height1 - height2 - height3;
    $("main#log").height(new_height);
  }, 100);
}


function set_transient_chart_height()
{
  setTimeout(function() {
    let total_height = $(window).height();
    let height1 = $("nav.navbar").outerHeight();
    let height2 = $("div#tab_interface > div.tabs").outerHeight();
    let new_height = total_height - height1 - height2 - 10;
    $("div#div_transient_chart").height(new_height);
  }, 100);
}


function set_spatial_chart_height()
{
  setTimeout(function() {
    let total_height = $(window).height();
    let height1 = $("nav.navbar").outerHeight();
    let height2 = $("div#tab_interface > div.tabs").outerHeight();
    let new_height = total_height - height1 - height2 - 10;
    let new_width = $("div#div_spatial_chart").innerWidth();
    $("div#div_spatial_chart").height(new_height);
    $("canvas#spatial_chart").prop('height', new_height);
    $("canvas#spatial_chart").prop('width', new_width);
    heat_map(10, 45);
  }, 100);
}


$(window).resize(function() {
  // $("div#main").css("margin-top", "" + $("nav.nav").height() +"px");
  set_terminal_height();
  set_transient_chart_height();
  set_spatial_chart_height();
});

$(document).ready(function () {
  $(".js-modal-trigger").click(function(){
    const modal = $(this).data('target');
    let target = $("#"+modal+'.modal');
    target.addClass('is-active');
  });

  $('.modal-background, .modal-close, .modal-card-head .delete, .modal-card-foot .button').click(function(){
    $(this).closest('.modal').removeClass('is-active');
  });

  $(document).on( "keydown", function(event) {
    if (event.which == 27)
    { // close any modal upon ESC-key press...
      $(".modal").removeClass('is-active');
    }
  });


  $(".checkmark.default_off").prop('checked', false);

  // navbar main buttons-clicks
  $("nav.navbar > div.navbar-menu > div.navbar-start > a.navbar-item").click(function() {
    set_terminal_height();
    set_transient_chart_height();
    set_spatial_chart_height();

    $(this).siblings().removeClass("is-active");
    $(this).addClass("is-active");
    $("div#tabs_main > div").removeClass("is-active");
    $("div#tabs_main > div#"+ $(this).attr('id')).addClass("is-active");
  });

  $("#btn_open_port").click(function () {
    $("div.navbar-start>a#tab_interface").trigger("click");
    set_terminal_height();
    set_transient_chart_height()
    set_spatial_chart_height();
  });

  $("#btn_close_port").click(function () {
    $("div.tabs>a#tab_interface").trigger("click");
    set_terminal_height();
    set_transient_chart_height();
    set_spatial_chart_height();
  });

  $("div.tabs>ul>li").click(function() {
    set_terminal_height();
    set_transient_chart_height();
    set_spatial_chart_height();
    $(this).siblings().removeClass("is-active");
    $(this).addClass("is-active");
    $(this).parent().parent().siblings("div.tab_content").removeClass("is-active");
    $(this).parent().parent().siblings("div.tab_content#"+ $(this).attr('id')).addClass("is-active");
  });

});



function get_date_time() {
        var now     = new Date();
        var year    = now.getFullYear();
        var month   = now.getMonth()+1;
        var day     = now.getDate();
        var hour    = now.getHours();
        var minute  = now.getMinutes();
        var second  = now.getSeconds();
        var ms      = now.getMilliseconds();
        if(month.toString().length == 1) {
             month = '0'+month;
        }
        if(day.toString().length == 1) {
             day = '0'+day;
        }
        if(hour.toString().length == 1) {
             hour = '0'+hour;
        }
        if(minute.toString().length == 1) {
             minute = '0'+minute;
        }
        if(second.toString().length == 1) {
             second = '0'+second;
        }
        if(ms.toString().length == 1) {
             ms = '00'+ms;
        }
        if(ms.toString().length == 2) {
             ms = '0'+ms;
        }
        var dateTime = year+'/'+month+'/'+day+' '+hour+':'+minute+':'+second+'.'+ms;
         return dateTime;
}

var last_chart_update_time = 0;

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

var startTime, endTime;

function start() {
  startTime = new Date();
};

function end() {
  endTime = new Date();
  var timeDiff = endTime - startTime; //in ms
  console.log(timeDiff + " milliseconds");
}


function hex_to_rgba(hex, a){
    var c;
    if(/^#([A-Fa-f0-9]{3}){1,2}$/.test(hex)){
        c= hex.substring(1).split('');
        if(c.length== 3){
            c= [c[0], c[0], c[1], c[1], c[2], c[2]];
        }
        c= '0x'+c.join('');
        return 'rgba('+[(c>>16)&255, (c>>8)&255, c&255].join(',')+','+a+')';
    }
    throw new Error('Bad Hex');
}

// img_dat is a imageData object,
// x,y are floats in the original coordinates
// Returns the pixel colour at that point as an array of RGBA
// Will copy last pixel's colour
// https://stackoverflow.com/a/46249246/2491604
function get_pixel_value(img_dat, x,y, result = []){
  let i;
  // clamp and floor coordinate
  const ix1 = (x < 0 ? 0 : x >= img_dat.width ? img_dat.width - 1 : x)| 0;
  const iy1 = (y < 0 ? 0 : y >= img_dat.height ? img_dat.height - 1 : y) | 0;
  // get next pixel pos
  const ix2 = ix1 === img_dat.width -1 ? ix1 : ix1 + 1;
  const iy2 = iy1 === img_dat.height -1 ? iy1 : iy1 + 1;
  // get interpolation position
  const xpos = x % 1;
  const ypos = y % 1;
  // get pixel index
  let i1 = (ix1 + iy1 * img_dat.width) * 4;
  let i2 = (ix2 + iy1 * img_dat.width) * 4;
  let i3 = (ix1 + iy2 * img_dat.width) * 4;
  let i4 = (ix2 + iy2 * img_dat.width) * 4;

  // to keep code short and readable get data alias
  const d = img_dat.data;

  // interpolate x for top and bottom pixels
  for(i = 0; i < 3; i ++){
    const c1 = (d[i2] * d[i2++] - d[i1] * d[i1]) * xpos + d[i1] * d[i1 ++];
    const c2 = (d[i4] * d[i4++] - d[i3] * d[i3]) * xpos + d[i3] * d[i3 ++];

    // now interpolate y
    result[i] = Math.sqrt((c2 - c1) * ypos + c1);
  }

  // and alpha is not logarithmic
  const c1 = (d[i2] - d[i1]) * xpos + d[i1];
  const c2 = (d[i4] - d[i3]) * xpos + d[i3];
  result[3] = (c2 - c1) * ypos + c1;
  return result;
}


function sent_command(command)
{
  const sent_event = new CustomEvent('sent', { detail: command });
  div = document.querySelector('#sent_data');
  div.dispatchEvent(sent_event);
}


$(document).ready(function () {
  $("#serial_terminal").html("<br>");
  $('#serial_terminal').prop('readonly', true);
  $("#serial_send").text("");
  $("#serial_send").prop('disabled', true); // disable input entry!
  $("#btn_open_port").removeClass("is-light");
  $("div#main button.button").prop('disabled', true); // disable input entry!
  $("button.button.cmd").one('click', function () {
    button_action(this);
  });
  // update i2c frequency at the master (Configure Hub, ch)
  $('select.combo_i2c_freq').on('change', function() {
    $('select.combo_i2c_freq').val(this.value);
    sent_command("+ch:I2C_FREQ="+this.value+"\n");
  });

  if (!("serial" in navigator)) {
    // The Web Serial API is NOT supported.
    $('button').not('.keep').prop('disabled', true); // disable all buttons!
    $("#serial_terminal").append('<pre>==> This page uses "Web Serial API".</pre><br>');
    $("#serial_terminal").append('<pre>https://developer.mozilla.org/en-US/docs/Web/API/Web_Serial_API</pre><br>');
    $("#serial_terminal").append('<pre> </pre><br>');
    $("#serial_terminal").append('<pre>Your browser/OS does NOT support the serial port!</pre><br>');
    $("#serial_terminal").append('<pre>Please use browser:</pre><br>');
    $("#serial_terminal").append('<pre> - Chrome</pre><br>');
    $("#serial_terminal").append('<pre> - Edge</pre><br>');
    $("#serial_terminal").append('<pre> - Opera</pre><br>');
    $("#serial_terminal").append('<pre>with an operating system:</pre><br>');
    $("#serial_terminal").append('<pre> - Linux</pre><br>');
    $("#serial_terminal").append('<pre> - ChromeOS</pre><br>');
    $("#serial_terminal").append('<pre> - macOS</pre><br>');
    $("#serial_terminal").append('<pre> - Windows 10+</pre><br>');
    return;
  }
  has_serial = true;
  $("#lbl_selected_port").text(selected_port.usbVendorId + '/' + selected_port.usbProductId);

  div = document.querySelector('#receive_data');

  // show receiving data in terminal...
  div.addEventListener('receive', (e) => {
    var value_str = dec.decode(e.detail).replaceAll('\r', '');
    let dt = get_date_time();
    if (!($("#chk_add_datetime").is(":checked")))
    {
      dt = '';
    }

    let values = value_str.split("\n");

    // encode html tags...
    for (var i = values.length - 1; i >= 0; i--)
    {
      values[i] = $('<div>').text(values[i]).html();
    }

    let has_line_ending = false;
    if (values[values.length-1] === '')
    {
      has_line_ending = true;
      values.pop();
    }

    let term = $("#serial_terminal");

    let term_html = term.html();
    if (term_html.length > 50000)
    {
      let last_part = term_html.split("<br>").slice(-1);
      if (last_part === '')
      {
        term.empty();
        term.append("<br>");
      } else
      {
        term.html(last_part);
      }
      term_html = term.html();
    }
    if (term.html().endsWith("<br>"))
    {
      term.append("<pre>"+dt+' -> </pre>');
    }
    term.append("<pre>" + values.join("</pre><br><pre>"+dt+' -> ') + "</pre>");
    if (has_line_ending)
    {
      term.append("<br>");
    }

    if ($("#chk_auto_scroll").is(":checked"))
    {
      term.scrollTop(term[0].scrollHeight);
    }
  }, false);

  // listener to generate recieve_line events.
  div.addEventListener('receive', (e) => {
    var value_str = dec.decode(e.detail);
    value_str = value_str.replaceAll('\r', '');

    let lines = value_str.split("\n");

    let complete_lines = lines.slice(0, -1);
    if (complete_lines.length > 0)
    {
      complete_lines[0] = receive_buffer + complete_lines[0];
      receive_buffer = "";
    }

    for (let i=0; i<complete_lines.length; i++)
    {
      const recieve_line_event = new CustomEvent('receive_line', { detail: complete_lines[i] });
      div.dispatchEvent(recieve_line_event);
    }

    receive_buffer += lines.slice(-1);

  }, false);

  // listener to enable buttons
  div.addEventListener('receive_line', (e) => {
    let line = e.detail;
    if (line.startsWith("dis:"))
    {
      let items = line.split(":");
      let sa = items[1];
      let disabled = Number(items[2]);
      $("div[data-sa='"+sa+"']>input.slave_enable").prop('checked', disabled ? false : true);
    }
  });

  // listener to update the chart
  div.addEventListener('receive_line', (e) => {
    if ($("#chk_transient_enable").is(":checked"))
    {
      let line = e.detail;

      if (line.startsWith ("@") || line.startsWith ("mv:"))
      { // update chart
        //@5A:01:mv:5A:00105147:23.25,24.77
        //mv:5A:00105147:23.25,24.77
        let start = 0;
        if (line.startsWith ("@"))
        {
          start = 7;
        }
        let items = line.slice(start).split(":");
        let time = items[2];
        let sa = items[1];
        let values = items[3].split(",");
        time = Number(time) / 1000;
        for (let i=0; i<values.length; i++)
        {
          values[i] = Number(values[i]);
        }

        if (!(sa in connected_slaves))
        {
          sent_command("ls\n");
          sleep(10).then(() => {
            sent_command("cs:"+sa+"\n");
          });

          return;
        }

        if (!('device' in connected_slaves[sa]))
        {
          sent_command("ls\n");
          sleep(10).then(() => {
            sent_command("cs:"+sa+"\n");
          });
          return;
        }

        if (!('cs' in connected_slaves[sa]))
        {
          sent_command("cs:"+sa+"\n");
          return;
        }

        if (items[3] == "FAIL")
        {
          let c = document.getElementById("transient_chart");
          let ctx = c.getContext('2d');
          console.log("FAIL!", items[4]);
          ctx.fillStyle = "black";
          ctx.font = "20px Courier New";
          ctx.fillText("FAIL: @"+sa+":"+connected_slaves[sa].device+" =>"+items[4], 20, 50);
          return;
        }

        if (!('transient_chart' in connected_slaves[sa]))
        {
          connected_slaves[sa]['transient_chart'] = {};
          if (Array.isArray(connected_slaves[sa].cs.MV_HEADER))
          {
            connected_slaves[sa]['transient_chart']['trace_i'] = [];
            let len = connected_slaves[sa].cs.MV_HEADER.length;
            if (len > 192)
            {
              len = 2;
            }
            for (let i=0; i<len; i++)
            {
              let d = connected_slaves[sa].device;
              let h = connected_slaves[sa].cs.MV_HEADER[i];
              let u = connected_slaves[sa].cs.MV_UNIT[i];
              // let r = connected_slaves[sa].cs.MV_RES[i];
              let trace_i = transient_chart.data.datasets.length;
              dataset = {label: d + "@" + sa + ": " + h + " [" + u + "]",
                         data: [],
                         backgroundColor: hex_to_rgba(color_palette[trace_i%color_palette.length], 0.2),
                         borderColor: hex_to_rgba(color_palette[trace_i%color_palette.length], 1),
                         borderWidth: 1,
                };
              connected_slaves[sa]['transient_chart']['trace_i'].push(trace_i);
              transient_chart.data.datasets.push(dataset);
            }
          } else
          { // only one trace for this slave device.
            let d = connected_slaves[sa].device;
            let h = connected_slaves[sa].cs.MV_HEADER;
            let u = connected_slaves[sa].cs.MV_UNIT;
            // let r = connected_slaves[sa].cs.MV_RES;
            let trace_i = transient_chart.data.datasets.length;
            dataset = {label: d + "@" + sa + ": " + h + " [" + u + "]",
                       data: [],
                       backgroundColor: hex_to_rgba(color_palette[trace_i%color_palette.length], 0.2),
                       borderColor: hex_to_rgba(color_palette[trace_i%color_palette.length], 1),
                       borderWidth: 1,
              };
            connected_slaves[sa]['transient_chart']['trace_i'] = trace_i;
            transient_chart.data.datasets.push(dataset);
          }
        }

        for (let i=0; i<connected_slaves[sa].transient_chart.trace_i.length; i++)
        {
          let trace_i = connected_slaves[sa].transient_chart.trace_i[i];
          let y = values[i];
          if (i > 0)
          {
            if (values.length === 193)
            {
              y = values[1+86];
            }
            if (values.length === 769)
            {
              y = values[1+384];
            }
          }
          transient_chart.data.datasets[trace_i].data.push({x:time, y:y});
        }

        // todo: truncate of data needs to take longest period of time for 100 samples, then cut on time.
        for (let index = 0; index < transient_chart.data.datasets.length; ++index)
        {
          while (transient_chart.data.datasets[index].data.length > 100)
          {
            transient_chart.data.datasets[index].data = transient_chart.data.datasets[index].data.slice(1);
          }
        }

        let now = new Date();
        var time_diff = now - last_chart_update_time;
        if (time_diff > 50)
        {
          let x_min = transient_chart.data.datasets[0].data[0].x;
          let x_max = x_min;
          for (let index = 0; index < transient_chart.data.datasets.length; ++index)
          {
            for (let j = 0; j < transient_chart.data.datasets[index].data.length; j++)
            {
              if (x_min > transient_chart.data.datasets[index].data[j].x)
              {
                x_min = transient_chart.data.datasets[index].data[j].x;
              }
              if (x_max < transient_chart.data.datasets[index].data[j].x)
              {
                x_max = transient_chart.data.datasets[index].data[j].x;
              }
            }
          }
          transient_chart.options.scales.x.min = x_min;
          transient_chart.options.scales.x.max = x_max;

          transient_chart.update();
          last_chart_update_time = now;
        }
      }
    }
  }, false);


  // spatial chart updater
  div.addEventListener('receive_line', (e) => {
    if ($("#chk_spatial_enable").is(":checked") &&
        $("#tab_int_spatial").hasClass("is-active")
       )
    {
      let line = e.detail;


      if (line.startsWith ("@") || line.startsWith ("mv:"))
      { // update chart
        //@5A:01:mv:5A:00105147:23.25,24.77
        //mv:5A:00105147:23.25,24.77
        let start = 0;
        if (line.startsWith ("@"))
        {
          start = 7;
        }
        let items = line.slice(start).split(":");
        let time = items[2];
        let sa = items[1];
        let values = items[3].split(",");
        time = Number(time) / 1000;
        for (let i=0; i<values.length; i++)
        {
          values[i] = Number(values[i]);
        }

        let c = document.getElementById("spatial_chart");
        let ratio = c.width / c.height;
        let ctx = c.getContext('2d');
        let interpolation = Number($("#combo_spatial_interpolation").find(":selected").val());
        let orientation = Number($("#combo_spatial_orientation").find(":selected").val());
        let mirror = $("#combo_spatial_mirror").find(":selected").val();

        if (items[3] == "FAIL")
        {
          console.log("FAIL!", items[4]);
          ctx.fillStyle = "black";
          ctx.font = "20px Courier New";
          ctx.fillText("FAIL:"+items[4], 20, 50);
          return;
        }

        if (spatial_previous_orientation != orientation)
        {
          ctx.fillStyle = "white";
          ctx.fillRect(0,0,c.width,c.height); // start with white canvas!
          spatial_previous_orientation = orientation;
        }

        if (!(sa in connected_slaves))
        {
          sent_command("ls\n");
          sleep(10).then(() => {
            sent_command("cs:"+sa+"\n");
          });
          return;
        }
        if (!('device' in connected_slaves[sa]))
        {
          sent_command("ls\n");
          sleep(10).then(() => {
            sent_command("cs:"+sa+"\n");
          });
          return;
        }

        if (!('cs' in connected_slaves[sa]))
        {
          sent_command("cs:"+sa+"\n");
          return;
        }

        let ta = values[0];
        let to = values.slice(1);
        let rows = 24;
        let cols = 32;
        let is_supported = false;

        if (connected_slaves[sa].device == "MLX9064x")
        {
          sent_command("ls\n");
          sent_command("cs:"+sa+"\n");
          return;
        }

        if (connected_slaves[sa].device == "MLX90640")
        {
          is_supported = true;
        }
        if (connected_slaves[sa].device == "MLX90641")
        {
          rows = 12;
          cols = 16;
          is_supported = true;
        }
        if (!is_supported)
        {
          console.log("data for not supported device found", connected_slaves[sa].device);
          return;
        }

        let img_data = ctx.createImageData(cols, rows);
        let t_min_current = Math.min.apply(null, to);
        let t_max_current = Math.max.apply(null, to);
        if (isNaN(t_min_current)) { t_min_current = t_min; }
        if (isNaN(t_max_current)) { t_max_current = t_max; }

        if ((t_max_current - t_min_current) < 10) // set min 10C difference in the scaling!
        {
          let diff = t_max_current - t_min_current;
          t_min_current -= (10-diff)/2;
          t_max_current += (10-diff)/2;
        }

        if (t_min == null) { t_min = t_min_current; }
        if (t_max == null) { t_max = t_max_current; }

        if (isNaN(t_min)) { t_min = t_min_current; }
        if (isNaN(t_max)) { t_max = t_max_current; }

        t_min = (t_min * 19/20) + (t_min_current / 20);
        t_max = (t_max * 19/20) + (t_max_current / 20);

        let to_normalized = [];
        for (let i=0; i<to.length; i++)
        {
          let normalized = Math.round((to[i] - t_min) / (t_max-t_min) * 1024);
          if (normalized > 1023) { normalized = 1023; }
          if (normalized <    0) { normalized =    0; }
          to_normalized.push(normalized);
        }
        for (let i=0; i<img_data.data.length/4; i+=1)
        {
          img_data.data[i*4+0] = heat_map_gradient_colors[to_normalized[i]*4+0];
          img_data.data[i*4+1] = heat_map_gradient_colors[to_normalized[i]*4+1];
          img_data.data[i*4+2] = heat_map_gradient_colors[to_normalized[i]*4+2];
          img_data.data[i*4+3] = heat_map_gradient_colors[to_normalized[i]*4+3];
        }

        // orientation
        if (orientation == 180)
        {
          let img_data2 = ctx.createImageData(cols, rows);
          for (let y=0; y<img_data.height; y++)
          {
            for (let x=0; x<img_data.width; x++)
            {
              img_data2.data[(y*img_data.width+(img_data.width-x-1))*4+0] = img_data.data[((img_data.height-y-1)*img_data.width+x)*4+0];
              img_data2.data[(y*img_data.width+(img_data.width-x-1))*4+1] = img_data.data[((img_data.height-y-1)*img_data.width+x)*4+1];
              img_data2.data[(y*img_data.width+(img_data.width-x-1))*4+2] = img_data.data[((img_data.height-y-1)*img_data.width+x)*4+2];
              img_data2.data[(y*img_data.width+(img_data.width-x-1))*4+3] = img_data.data[((img_data.height-y-1)*img_data.width+x)*4+3];
            }
          }
          img_data = img_data2;
        }
        if (orientation == 90)
        {
          let img_data2 = ctx.createImageData(rows, cols);
          for (let y=0; y<img_data2.height; y++)
          {
            for (let x=0; x<img_data2.width; x++)
            {
              img_data2.data[(y*img_data2.width+x)*4+0] = img_data.data[((img_data.height-x-1)*img_data.width+y)*4+0];
              img_data2.data[(y*img_data2.width+x)*4+1] = img_data.data[((img_data.height-x-1)*img_data.width+y)*4+1];
              img_data2.data[(y*img_data2.width+x)*4+2] = img_data.data[((img_data.height-x-1)*img_data.width+y)*4+2];
              img_data2.data[(y*img_data2.width+x)*4+3] = img_data.data[((img_data.height-x-1)*img_data.width+y)*4+3];
            }
          }
          img_data = img_data2;
        }
        if (orientation == 270)
        {
          let img_data2 = ctx.createImageData(rows, cols);
          for (let y=0; y<img_data2.height; y++)
          {
            for (let x=0; x<img_data2.width; x++)
            {
              img_data2.data[((img_data2.height-y-1)*img_data2.width+(img_data2.width-x-1))*4+0] = img_data.data[((img_data.height-x-1)*img_data.width+y)*4+0];
              img_data2.data[((img_data2.height-y-1)*img_data2.width+(img_data2.width-x-1))*4+1] = img_data.data[((img_data.height-x-1)*img_data.width+y)*4+1];
              img_data2.data[((img_data2.height-y-1)*img_data2.width+(img_data2.width-x-1))*4+2] = img_data.data[((img_data.height-x-1)*img_data.width+y)*4+2];
              img_data2.data[((img_data2.height-y-1)*img_data2.width+(img_data2.width-x-1))*4+3] = img_data.data[((img_data.height-x-1)*img_data.width+y)*4+3];
            }
          }
          img_data = img_data2;
        }

        // mirroring
        if (mirror == 'x')
        {
          let img_data2 = ctx.createImageData(img_data.width, img_data.height);
          for (let y=0; y<img_data2.height; y++)
          {
            for (let x=0; x<img_data2.width; x++)
            {
              img_data2.data[((img_data2.height-y-1)*img_data2.width+x)*4+0] = img_data.data[(y*img_data.width+x)*4+0];
              img_data2.data[((img_data2.height-y-1)*img_data2.width+x)*4+1] = img_data.data[(y*img_data.width+x)*4+1];
              img_data2.data[((img_data2.height-y-1)*img_data2.width+x)*4+2] = img_data.data[(y*img_data.width+x)*4+2];
              img_data2.data[((img_data2.height-y-1)*img_data2.width+x)*4+3] = img_data.data[(y*img_data.width+x)*4+3];
            }
          }
          img_data = img_data2;
        }
        if (mirror == 'y')
        {
          let img_data2 = ctx.createImageData(img_data.width, img_data.height);
          for (let y=0; y<img_data2.height; y++)
          {
            for (let x=0; x<img_data2.width; x++)
            {
              img_data2.data[(y*img_data2.width+(img_data2.width-x-1))*4+0] = img_data.data[(y*img_data.width+x)*4+0];
              img_data2.data[(y*img_data2.width+(img_data2.width-x-1))*4+1] = img_data.data[(y*img_data.width+x)*4+1];
              img_data2.data[(y*img_data2.width+(img_data2.width-x-1))*4+2] = img_data.data[(y*img_data.width+x)*4+2];
              img_data2.data[(y*img_data2.width+(img_data2.width-x-1))*4+3] = img_data.data[(y*img_data.width+x)*4+3];
            }
          }
          img_data = img_data2;
        }
        if (mirror == 'xy')
        {
          let img_data2 = ctx.createImageData(img_data.width, img_data.height);
          for (let y=0; y<img_data2.height; y++)
          {
            for (let x=0; x<img_data2.width; x++)
            {
              img_data2.data[((img_data2.height-y-1)*img_data2.width+(img_data2.width-x-1))*4+0] = img_data.data[(y*img_data.width+x)*4+0];
              img_data2.data[((img_data2.height-y-1)*img_data2.width+(img_data2.width-x-1))*4+1] = img_data.data[(y*img_data.width+x)*4+1];
              img_data2.data[((img_data2.height-y-1)*img_data2.width+(img_data2.width-x-1))*4+2] = img_data.data[(y*img_data.width+x)*4+2];
              img_data2.data[((img_data2.height-y-1)*img_data2.width+(img_data2.width-x-1))*4+3] = img_data.data[(y*img_data.width+x)*4+3];
            }
          }
          img_data = img_data2;
        }

        // interpolate img_data
        if (interpolation > 0)
        {
          const interpolate_factor = interpolation;
          let img_data2 = ctx.createImageData(img_data.width*interpolate_factor, img_data.height*interpolate_factor);
          const res = new Uint8ClampedArray(4);
          for(var y = 0; y < img_data2.height; y++){
            for(var x = 0; x < img_data2.width; x++){
              get_pixel_value(img_data, x / interpolate_factor, y / interpolate_factor, res);
              img_data2.data.set(res,(x + y * img_data2.width) * 4);
            }
          }
          // ctx.putImageData(img_data2,0,0);
          img_data = img_data2;
        }

        let zoom_x = c.width / img_data.width;
        let zoom_y = c.height / img_data.height;

        let zoom=Math.min(zoom_x, zoom_y);

        for (var x=0;x<img_data.width;++x){
          for (var y=0;y<img_data.height;++y){
            // Find the starting index in the one-dimensional image data
            var i = (y*img_data.width + x)*4;
            var r = img_data.data[i  ];
            var g = img_data.data[i+1];
            var b = img_data.data[i+2];
            var a = img_data.data[i+3];
            ctx.fillStyle = "rgba("+r+","+g+","+b+","+(a/255)+")";
            ctx.fillRect(x*zoom,y*zoom,zoom,zoom);
          }
        }
        heat_map(t_min, t_max); // update the colormap scaling!
      }
    }
  }, false);

  // listener to update userinterface with current configuration of the hub
  div.addEventListener('receive_line', (e) => {
    let line = e.detail;
    if (line.startsWith ("ch:"))
    { // index slave
      let tmp = line.split(":");
      // old format:
      //   ch:FORMAT=DEC(0)
      //   ch:I2C_FREQ=400kHz(1)
      // new format as of V1.4
      //   FORMAT=0(DEC)
      //   I2C_FREQ=2(1MHz)
      let key_value = tmp[1].split('=');
      if (key_value[0] == 'I2C_FREQ')
      {
        let value_list = ["F100k", "F400k", "F1M", "F50k", "F20k", "F10k"];

        // try the old format
        let index = Number(key_value[1].split(/[\(\)]/)[1]);
        let value = value_list[index];
        if (value !== undefined)
        {
          $('select.combo_i2c_freq').val(value);
        } else
        { // give the new format a shot
          let index = Number(key_value[1].split(/[\(\)]/)[0]);
          let value = value_list[index];
          if (value !== undefined)
          {
            $('select.combo_i2c_freq').val(value);
          }
        }
      }
    }
  }, false);

  // listener to update slave list
  div.addEventListener('receive_line', (e) => {
    let line = e.detail;
    if (line.startsWith ("ls:") || line.startsWith ("scan:"))
    { // index slave
      // "ls:5A:01,00,00,MLX90614"
      // "scan:5A:01,00,00,MLX90614"
      let tmp = line.split(":");
      let sa = tmp[1];
      let drv_dev = tmp[2].split(",");
      let drv = Number(drv_dev[0]);
      let raw = Number(drv_dev[1]);
      let disabled = Number(drv_dev[2]);
      let device = drv_dev[3];

      connected_slaves[sa] = {drv: drv, device: device, raw: raw, disabled: disabled};

      new_div = $($(".slave_transient_chart").html()).attr("data-sa", sa);
      new_div.children("#btn_config").text("@"+sa+":"+device);
      new_div.children("#slave_enable").prop('checked', disabled ? false : true);
      new_div.children("button.button.cmd").one('click', function () {
        button_action(this);
      });
      new_div.children("input.slave_enable").change(function () {
        checkbox_slave_enable(this);
      });
      new_div.appendTo("#transient_chart_slaves");

      new_div = $($(".slave_spatial_chart").html()).attr("data-sa", sa);
      new_div.children("#btn_config").text("@"+sa+":"+device);
      new_div.children("#slave_enable").prop('checked', disabled ? false : true);
      new_div.children("button.button.cmd").one('click', function () {
        button_action(this);
      });
      new_div.children("input.slave_enable").change(function () {
        checkbox_slave_enable(this);
      });
      new_div.appendTo("#spatial_chart_slaves");

      new_div = $($(".slave_terminal").html()).attr("data-sa", sa);
      new_div.children("#btn_config").text("@"+sa+":"+device);
      new_div.children("#slave_enable").prop('checked', disabled ? false : true);
      new_div.children("button.button.cmd").one('click', function () {
        button_action(this);
      });
      new_div.children("input.slave_enable").change(function () {
        checkbox_slave_enable(this);
      });
      new_div.appendTo("#terminal_slaves");
    }

    if (line.startsWith ("cs:"))
    { // parse config of slave
      // "cs:3A:DRV=05"
      let tmp = line.split(":");
      let sa = tmp[1];
      if (sa == "FAIL")
      {
        return;
      }
      let kv = tmp[2];
      if (kv == "RO")
      {
        kv = tmp[3];
      }
      let key_value = kv.split("=");
      let key = key_value[0];
      let value = key_value[1].split(",");
      let description = "";

      // check if certain values are 'coding' an array
      // cs:33:RO:MV_HEADER=TA,TO[768]
      for (let i = value.length-1; i>=0; i--)
      {
        let tmp = value[i].split(/[\[\]]/);
        if (tmp.length == 3)
        {
          if (!isNaN(tmp[1]))
          {
            let len = Number(tmp[1]);
            if (len <= 0) { continue; }
            let arr = Array.apply(null, Array(len)).map(function (x, i) { return tmp[0]; })
            if (tmp[0].endsWith("_"))
            {
              value[i] = tmp[0]+'0';
            } else {
              value[i] = tmp[0];
            }
            for (let j=1; j<len; j++)
            {
              let suffix = '';
              if (tmp[0].endsWith("_"))
              {
                suffix = String(j);
              }
              value.splice(i+j, 0, tmp[0]+suffix);
            }
          }
        }
      }


      // if value is not an array, make it a variable....
      if (value.length == 1)
      {
        // only when value is not an array it can contain a description...
        // cs:3A:RO:I2C=0(3V3)
        value = value[0].split(/[\(\)]/);
        if (value.length > 1)
        {
          description = value[1];
        }
        value = value[0];
      }


      // convert value-string to numbers
      if (!["SA"].includes(key))
      {
        try
        {
          if (Array.isArray(value))
          {
            for (let i = 0; i<value.length; i++)
            {
              if (!isNaN(value[i]))
              {
                let v = Number(value[i]);
                value[i] = v;
              }
            }
          } else
          {
            if (!isNaN(value))
            {
              let v = Number(value);
              value = v;
            }
          }
        } catch (error)
        {
          // ignore error, keep value as a string.
        }
      }

      if (sa in connected_slaves === false)
      {
        connected_slaves[sa] = {};
      }
      if ('cs' in connected_slaves[sa] === false)
      {
        connected_slaves[sa]['cs'] = {};
      }
      connected_slaves[sa]['cs'][key] = value;
      if (description !== "")
      {
        connected_slaves[sa]['cs'][key+'_description'] = description;
      }
    }
  }, false);


  div = document.querySelector('#sent_data');
  div.addEventListener('sent', async (e) => {
    let value_str = e.detail;
    if (value_str.length === 1)
    {
      value_str += '\n';
    }
    let value_byte = enc.encode(value_str);

    dt = get_date_time();
    if (!($("#chk_add_datetime").is(":checked")))
    {
      dt = '';
    }

    let term = $("#serial_terminal");

    if (!(term.html().endsWith("<br>")))
    {
      term.append("<br>");
    }

    term.append("<pre>" + dt + ' <- ' + value_str.replaceAll('\n','') + "</pre><br>");
    if ($("#chk_auto_scroll").is(":checked"))
    {
      term.scrollTop(term[0].scrollHeight);
    }

    await writer.write(value_byte);

  }, false);

  // reset the connected_slave container when we detect a scan or ls command.
  div.addEventListener('sent', async (e) => {
    let value_str = e.detail;
    if (['5', 'scan\n', '5\n', 'ls\n'].includes(value_str))
    {
      connected_slaves = {};
      transient_chart.data.datasets = []; // also reset the transient plot data config, as trace_i became invalid
      $("#terminal_slaves").html(""); // empty slave buttons
      $("#transient_chart_slaves").html(""); // empty slave buttons
      $("#spatial_chart_slaves").html(""); // empty slave buttons
    }

  }, false);

  const ctx = document.getElementById('transient_chart');
  transient_chart = new Chart(ctx, {
      type: 'scatter',
      data: {
          datasets: []
      },
      options: {
          plugins: {
            title: {
              display: false,
              text: 'Transient measured sensors values',
            },
            legend: {
              display: true,
              title: {
                display: false,
                text: 'Legend Title',
              }
            }
          },
          maintainAspectRatio: false,
          scales: {
              y: {
                  beginAtZero: false
              }
          }
      }
  });

  transient_chart.options.animation.duration = 0;
  transient_chart.options.scales.x.ticks.count = 8;
});

function pop_error(message)
{
  $('#error-popup').find('.modal-card-body').html('<p>'+message+'</p>');
  $("#error-popup").addClass('is-active');
}

async function serial_open_and_start_reading()
{
  keep_reading = true;
  try
  {
    await serial.open({ baudRate: 1000000 });

    dt = get_date_time();
    if (!($("#chk_add_datetime").is(":checked")))
    {
      dt = '';
    }

    let term = $("#serial_terminal");

    if (!(term.html().endsWith("<br>")))
    {
      term.append("<br>");
    }

    term.append("<pre>"+dt+" ## </pre>" + "<pre>" + "[connected]" + "</pre><br>");

    if ($("#chk_auto_scroll").is(":checked"))
    {
      term.scrollTop(term[0].scrollHeight);
    }
  } catch (error)
  {
    console.log(error);
    $("#serial_send").prop('disabled', true); // disable input entry!
    $("div#main button.button").prop('disabled', true); // disable input entry!
    $("#btn_open_port").removeClass("is-light");

    dt = get_date_time();
    if (!($("#chk_add_datetime").is(":checked")))
    {
      dt = '';
    }

    let term = $("#serial_terminal");

    if (!(term.html().endsWith("<br>")))
    {
      term.append("<br>");
    }

    term.append("<pre>"+dt+" ## </pre>" + "<pre>" + "[disconnected] " + error.toString() + "</pre><br>");
    pop_error(error.toString());

    if ($("#chk_auto_scroll").is(":checked"))
    {
      term.scrollTop(term[0].scrollHeight);
    }
    serial = null;
    return;
  }

  $("#serial_send").prop('disabled', false); // enable input entry!
  $("div#main button.button").prop('disabled', false); // disable input entry!
  $("#btn_open_port").addClass("is-light");

  while (serial.readable && keep_reading)
  {
    reader = serial.readable.getReader();
    writer = serial.writable.getWriter();
    // Listen to data coming from the serial device.
    try
    {
      {
        sent_command("mlx\n");
        sleep(10).then(() => {
          sent_command("bi\n");
          sleep(10).then(() => {
            sent_command("fv\n");
            sleep(10).then(() => {
              sent_command("ch\n");
              sleep(10).then(() => {
                sent_command("ls\n");
              });
            });
          });
        });
      }

      while (true) {
        const { value, done } = await reader.read();
        if (done) {
          // reader.cancel() has been called.
          reader.releaseLock();
          break;
        }
        const recieve_event = new CustomEvent('receive', { detail: value });
        div = document.querySelector('#receive_data');
        div.dispatchEvent(recieve_event);
      }
    } catch (error) {
      console.log(error);
      $("#serial_send").prop('disabled', true); // disable input entry!
      $("div#main button.button").prop('disabled', true); // disable input entry!
      $("#btn_open_port").removeClass("is-light");

      dt = get_date_time();
      if (!($("#chk_add_datetime").is(":checked")))
      {
        dt = '';
      }


      let term = $("#serial_terminal");

      if (!(term.html().endsWith("<br>")))
      {
        term.append("<br>");
      }

      term.append("<pre>"+dt+" ## </pre>" + "<pre>" + "[disconnected] " + error.toString() + "</pre><br>");

      if ($("#chk_auto_scroll").is(":checked"))
      {
        term.scrollTop(term[0].scrollHeight);
      }
    } finally {
      // Allow the serial port to be closed later.
      reader.releaseLock();
    }
  }
  writer.releaseLock();
  await serial.close();
  serial = null;
}


$("#lbl_selected_port").click(async () => {
  if (serial != null)
  {
    await close_port();
  }
  serial = await navigator.serial.requestPort();
  if (serial != null)
  {
    selected_port = serial.getInfo();
    $("#lbl_selected_port").text(selected_port.usbVendorId + '/' + selected_port.usbProductId);
  }

  closed_promise = serial_open_and_start_reading();
});


$("#btn_open_port").click(async () => {
  if (serial != null)
  {
    await close_port();
  }

  if (!('serial' in navigator))
  {
    pop_error($("#serial_terminal").html());
    return;
  }


  const ports = await navigator.serial.getPorts();
  for (let i = 0; i< ports.length; i++)
  {
    let port = ports[i];
    let port_info = port.getInfo();
    if ((port_info['usbProductId'] == selected_port['usbProductId']) &&
        (port_info['usbVendorId'] == selected_port['usbVendorId']))
    {
      serial = port;
      break;
    }
  }

  if (serial == null)
  {
    serial = await navigator.serial.requestPort({ filters: accepted_ports });
    if (serial != null)
    {
      selected_port = serial.getInfo();
      $("#lbl_selected_port").text(selected_port.usbVendorId + '/' + selected_port.usbProductId);
    }
  }

  closed_promise = serial_open_and_start_reading();
});

async function close_port()
{
  if (serial != null)
  {
  // User clicked a button to close the serial port.
    keep_reading = false;
    $("#serial_send").prop('disabled', true); // disable input entry!
    $("div#main button.button").prop('disabled', true); // disable input entry!
    $("#btn_open_port").removeClass("is-light");
    // Force reader.read() to resolve immediately and subsequently
    // call reader.releaseLock() in the loop example above.
    reader.cancel();
    await closed_promise;
  }
  serial = null;
}


$("#btn_close_port").click(async () => {
  await close_port();
});


function button_action(but)
{
  let sa = $(but).parent().data("sa");
  if (sa === undefined)
  {
    sent_command($(but).data("cmd") + "\n");
  } else
  {
    sent_command($(but).data("cmd") + ":" + sa + "\n");
  }
  sleep(100).then(() => {
    $(but).one('click', function () {
      button_action(but);
    });
  });
}

function checkbox_slave_enable(chkbox)
{
  let sa = $(chkbox).parent().data("sa");
  if (sa === undefined)
  {
    console.log("no slave address assigned; did nothing");
  } else
  {
    let disabled = $(chkbox).is(":checked");
    sent_command("dis:" + sa + ":" + (disabled ? "0" : "1") + "\n");
  }
}

$("#serial_send").on('keyup', function (val, key) {
  const single_char_cmd_list = [';', '!', '>', '<', '?', '1', '5'];
  // check if current value is a single character command
  current_entry = $(this).val();
  if (current_entry.length == 1)
  {
    if (single_char_cmd_list.includes(current_entry))
    { // yes! Only here we have to sent without enter/sent click
      sent_command(current_entry);
      $(this).val("");
    }
  }

  if (current_entry.length >= 1)
  {
    if (val.key === "Enter")
    { // Yes, we have an Enter! let's sent the current buffer!
      sent_command(current_entry + '\n');
      $(this).val("");
    }
  }
});


function heat_map(t_min, t_max)
{
  let c = document.getElementById("spatial_chart");
  let ratio = c.width / c.height;
  let ctx = c.getContext('2d');
  let keys = ["blue", "lightgreen", "gold", "red", "darkred"];

  if (ratio > (4/3))
  {
    let gr = ctx.createLinearGradient(0, 0, 0, c.height);
    keys = keys.reverse();

    // add color stops to gradient:
    for(let i = 0, key; key = keys[i]; i++) {
      gr.addColorStop(i / (keys.length-1), key);
    }

    ctx.fillStyle = gr;
    ctx.fillRect(c.width-50, 0, 50, c.height);

    ctx.fillStyle = "white";
    ctx.font = "10px Courier New";
    const t_step = (t_max - t_min) / (c.height / 308 * 20);
    for (let t = t_min; t<=t_max; t+=t_step)
    {//25 => y=c.height-10 && 35 => y=10
      let y = ((t_max-t) / (t_max-t_min) * (c.height-15)) + 10;
      ctx.fillText(t.toFixed(1)+"C", c.width-45, y);
    }

  } else {
    let gr = ctx.createLinearGradient(0, 0, c.width, 0);

    // add color stops to gradient:
    for(let i = 0, key; key = keys[i]; i++) {
      gr.addColorStop(i / (keys.length-1), key);
    }

    ctx.fillStyle = gr;
    ctx.fillRect(0, c.height-20, c.width, 20);

    ctx.fillStyle = "white";
    ctx.font = "10px Courier New";
    const t_step = (t_max - t_min) / (c.height / 400 * 10);
    for (let t = t_min; t<=t_max; t+=t_step)
    {//25 => y=c.height-10 && 35 => y=10
      let x = ((t - t_min) / (t_max-t_min) * (c.width-15))+3;
      ctx.fillText(t.toFixed(1)+"C", x, c.height-5);
    }
  }

  if (heat_map_gradient_colors == null)
  {
    c = document.createElement("canvas");
    c.width = 1024;
    c.height = 10;
    ctx = c.getContext('2d');
    let gr = ctx.createLinearGradient(0, 0, 1024, 0);
    for(let i = 0, key; key = keys[i]; i++) {
      gr.addColorStop(i / (keys.length-1), key);
    }
    ctx.fillStyle = gr;
    ctx.fillRect(0, 0, 1024, 1);

    let s = ctx.getImageData(0,0,1024,1);
    heat_map_gradient_colors = s.data;
  }
}


//<!--     <script src="./intel-hex.browser.js"></script> -->
//    <!-- <script src="https://unpkg.com/nrf-intel-hex"></script> -->
//
//
//
//function concatTypedArrays(a, b) { // a, b TypedArray of same type
//    var c = new (a.constructor)(a.length + b.length);
//    c.set(a, 0);
//    c.set(b, a.length);
//    return c;
//}
//
//function concatBytes(ui8a, byte) {
//    var b = new Uint8Array(1);
//    b[0] = byte;
//    return concatTypedArrays(ui8a, b);
//}
//
//
//if (0)
//{
//  let stick_ee = "ee:3A:2400,10,01,0100,DATA,F15A,3480,0000,0000,0013,4443,AB23,7587,A729,0021,0000,FE05,F9B2,0058,E0B4,04AA,0000,0000,1900,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,AE0B,004A,F9B2,0058,E242,03CD,86DC,FD14,0D54,FDCD,2A00,2A00,5939,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,8F5F,B4C4,4000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0006,001D,0002,12C0,0002,0960,AA91,1902,AA91,1902,2552,03A0,070B,820D,821D,FFFF,820D,821D,820D,821D,820D,821D,8201,BAE1,FFFF,8302,8310,8207,FFFF,8300,8312,830C,FFFF,FFFF,FFFF,FFFF,FFFF,FFFF,FFFF,FFFF,FFFF,FFFF,FFFF,FFFF";
//  let tmp = stick_ee.split(":");
//  let tmp2 = tmp.slice(0,2);
//  tmp2 = tmp2.concat(tmp[2].split(","));
//  let tmp3 = tmp2.map(item => parseInt(item, 16));
//  let tmp4 = tmp3.slice(7);
//  console.log(tmp4);
//  let tmp5 = new Uint8Array(0);
//  console.log(tmp5);
//  for (let i = 0; i < tmp4.length; ++i) {
//    const item = tmp4[i];
//    tmp5 = concatBytes(tmp5, item&0x000FF);
//    tmp5 = concatBytes(tmp5, (item&0x0FF00) >>> 8);
//  }
//  console.log(tmp5);
//
//  //const start_address = 0x4800;//parseInt(tmp3[2] * 2);
//  //let memMap = new MemoryMap({0x4800: tmp5});
//  //console.log(memMap.asHexString());
//
//}
//