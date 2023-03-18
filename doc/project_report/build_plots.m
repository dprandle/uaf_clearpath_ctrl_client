set(0, "defaulttextfontsize", 48)  % title
set(0, "defaultaxesfontsize", 32)  % axes labels
set(0, "defaultlinelinewidth", 4)

function avg = plot_vals(fname, zoom_in, subplot_n, zoom_in_2)
  T = dlmread(fname)

  keys = T(:,1)./1000
  vals = [T(:,2)./1024 T(:,4)./1024 T(:,3)./1024]

  elapsed = keys(end)
  mbit_factor = 8 / (1024*1024)
  total_mbits = (sum(T(:,2)) + sum(T(:,3)) + sum(T(:,4)))*mbit_factor
  avg = total_mbits / elapsed

  subplot_ind = 1

  subplot(subplot_n,1,subplot_ind)
  ++subplot_ind
  plot(keys, vals)
  xlabel ("Elapsed (S)")
  ylabel ("Size (KB)")
  legend ("Map", "Local Costmap", "Global Costmap Updates")

  if (subplot_n > 2)
    keys_zoomed_2 = T(zoom_in_2:end,1)./1000
    vals_zoomed_2 = [T(zoom_in_2:end,2)./1024 T(zoom_in_2:end,4)./1024 T(zoom_in_2:end,3)./1024]

    subplot(subplot_n,1,subplot_ind)
    ++subplot_ind
    plot(keys_zoomed_2, vals_zoomed_2)
    xlabel ("Elapsed (S)")
    ylabel ("Size (KB)")
    legend ("Map", "Local Costmap", "Global Costmap Updates")
  endif

  keys_zoomed = T(zoom_in(1):zoom_in(2),1)./1000
  vals_zoomed = [T(zoom_in(1):zoom_in(2),2)./1024 T(zoom_in(1):zoom_in(2),4)./1024 T(zoom_in(1):zoom_in(2),3)./1024]

  subplot(subplot_n,1,subplot_ind)
  plot(keys_zoomed, vals_zoomed)
  xlabel ("Elapsed (S)")
  ylabel ("Size (KB)")
  legend ("Map", "Local Costmap", "Global Costmap Updates")
endfunction

function plot_cam_vals(fname)
  T = dlmread(fname)

  keys = T(:,1)./1000
  client_info = [T(:,2) T(:,3)]
  bw_info = [T(:,4) T(:,5)]

  subplot(2,1,1)
  ax = plotyy(keys, bw_info, keys, client_info)
  xlabel ("Elapsed (S)")
  ylabel (ax(1), "Bandwidth (Mbps)")
  ylabel (ax(2), "Client Count")
  legend ("Current Bandwidth", "Average Bandwidth","Connections", "Viewing Camera")
  
  keys_zoomed = T(437:end,1)./1000
  client_info_zoomed = [T(437:end,2)]
  bw_info_zoomed = [T(437:end,4) T(437:end,5)]

  subplot(2,1,2)
  ax = plotyy(keys_zoomed, bw_info_zoomed, keys_zoomed, client_info_zoomed)
  xlabel ("Elapsed (S)")
  ylabel (ax(1), "Bandwidth (Mbps)")
  ylabel (ax(2), "Client Count")
  legend ("Current Bandwidth", "Average Bandwidth","Connections")
endfunction

figure(1, "position", [8000,2000,2500,800])
clf()
avg_raw = plot_vals('raw_map_server_data.csv', [462 533], 2)

figure(2, "position", [8000,2000,2500,1200])
clf()
avg_chg = plot_vals('change_map_server_data.csv', [478 551], 3, 5)

figure(3, "position", [8000,2000,2500,1400])
clf()
plot_cam_vals('cam_throttled_bandwidth.csv', [478 551], 3, 5)

