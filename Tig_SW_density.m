% Seawater equation of state
% CALCULATE SW DENSITY FROM S AND T: USE SECOND VERSION MORE RECENT
% online calculator for single point: http://science.widener.edu/~svanbram/seawater.html

% Millero, F. J. and Poisson, A. (1981). International one-atmosphere equation of state of seawater. 
% Deep Sea Research Part A. Oceanographic Research Papers 28, 625-629.

%returns values in kg/m^3, 1 m^3 = 1000L
%mean SW density should be roughly 1.025 kg/L
%NOTE: THESE VALUES WILL BE SLIGHTLY LOW FOR HIGH SALINITIES, 

%sal in ppt, temp in C (not K!!!)

tempmin = 8;
tempmax = 40;
salmin = 0.1; %ppt = g/kg
salmax = 140;
%--------------------------------------------------------


tempplot=(tempmin:.1:tempmax)';
temp = tempplot; % +  273.15;
salplot = (salmin:.1:salmax)';
sal=(salmin:.1:salmax)';





%----------------
%A = nan(length(temp),length(sal));
%B = nan(length(temp),length(sal));
%C = nan(length(temp),length(sal));
rho = nan(length(temp),length(sal));
for i = 1:length(temp)
    for j = 1:length(sal)
        A = .824493 - .0040899*(temp(i)) +  .000076438*((temp(i))^2) - .00000082467*((temp(i))^3) + .0000000053875*((temp(i))^4); 
        B = -.00572466 + .00010227*(temp(i)) - .0000016546*((temp(i))^2);
        C = .00048314; 
        rho0 = 999.842594 +.06793952*(temp(i)) - .009095290*(temp(i)^2) + .0001001685*(temp(i)^3) - .000001120083*(temp(i)^4) + .000000006536336*(temp(i)^5); 
        rho(i,j) = rho0 + A*sal(j) + B*(sal(j)^(3/2)) + C*sal(j);
    end
end
rho = rho/1000; %convert to kg/L from kg/m^3

temp2=repmat(tempplot,length(sal),1); %temp is rows of mg
sal2=reshape((repmat(salplot,1,length(tempplot))'),size(tempplot,1)*size(salplot,1),1); %sal is columns of mg
rho_array=reshape(rho,[size(rho,1)*size(rho,2),1]); %mg 501r x 1501c

%interpolation gride for surf plotting purposes only
[tempq,salq] = meshgrid(tempmin:((tempmax-tempmin)/100):tempmax, salmin:((salmax-salmin)/100):salmax);
rhoq = griddata(temp2,sal2,rho_array,tempq,salq); % *q interpolated values evenly spaced

figure
y=surf(tempq,salq,rhoq);
box on;
set(y,'LineStyle','none');
xlabel('Temperature (C)');ylabel('Salinity (ppt)');zlabel('Seawater density (kg/L)')  %OLD switch x and y labels b/c "x-axis" of mg matrix is actually at different sal
title('SW density (rho, kg/m^3) as function of S and T')


% ------- BELOW THIS POINT FOR SINGLE VALUE CHECKING AGAINST PUBLISHED rho-
% i=1; j=1;
% temp = 25.004;
% sal = 70.871;
% 
% A = .824493 - .0040899*(temp(i)) +  .000076438*((temp(i))^2) - .00000082467*((temp(i))^3) + .0000000053875*((temp(i))^4); 
% B = -.00572466 + .00010227*(temp(i)) - .0000016546*((temp(i))^2);
% C = .00048314; 
% rho0 = 999.842594 +.06793952*(temp(i)) - .009095290*(temp(i)^2) + .0001001685*(temp(i)^3) - .000001120083*(temp(i)^4) + .000000006536336*(temp(i)^5); 
% rho = rho0 + A*sal(j) + B*(sal(j)^(3/2)) + C*sal(j)



%-------------------------------------------------------------------------
%Millero and Huang + CORRIGENDUM (ORIGINAL COEFFICIENTS WERE WRONG!!)

%Millero, F. J. and Huang, F. (2009). The density of seawater as a function 
%of salinity (5 to 70 g kg?1) and temperature (273.15 to 363.15 K). Ocean Sci. 5, 91-100.

%Millero, F. J. and Huang, F. (2009). Corrigendum to "The density of seawater
%as a function of salinity (5 to 70 g kg?1) and temperature (273.15 to 363.15 K)" 
%published in Ocean Sci., 5, 91–100, 2009. Ocean Sci. 6, 379.

a0 = .8211458;
a1 = -.003959680;
a2 = .00007305182;
a3 = -.0000008282446;
a4 = .000000005386657;
a5 = 0;
b0 = -.006058307;
b1 = .00008265457;
b2 = -.000001077747;
c0 = .000526528;

rho = nan(length(temp),length(sal));
for k = 1:length(temp)
    for j = 1:length(sal)
        K = temp(k);% + 273.15;
        A = a0 + a1*(K) + a2*((K)^2) + a3*((K)^3) + a4*((K)^4) + a5*((K)^5);
        B = b0 + b1*(K) + b2*((K)^2);
        C = c0; 
        rho0 = 999.842594 +.06793952*(temp(k)) - .009095290*(temp(k)^2) + .0001001685*(temp(k)^3) - .000001120083*(temp(k)^4) + .000000006536336*(temp(k)^5); 
        rho(k,j) = (A*sal(j) + B*((sal(j))^1.5) + C*((sal(j))^2)) + rho0;
    end
end
rho = rho/1000; 

temp2=repmat(tempplot,length(sal),1); %temp is rows of mg
sal2=reshape((repmat(salplot,1,length(tempplot))'),size(tempplot,1)*size(salplot,1),1); %sal is columns of mg
rho_array=reshape(rho,[size(rho,1)*size(rho,2),1]); %mg 501r x 1501c

%interpolation gride for surf plotting purposes only
[tempq,salq] = meshgrid(tempmin:((tempmax-tempmin)/100):tempmax, salmin:((salmax-salmin)/100):salmax);
rhoq = griddata(temp2,sal2,rho_array,tempq,salq); % *q interpolated values evenly spaced

figure
y=surf(tempq,salq,rhoq);
box on;
set(y,'LineStyle','none');
xlabel('Temperature (C)');ylabel('Salinity (ppt)');zlabel('Seawater density (kg/L)')  %OLD switch x and y labels b/c "x-axis" of mg matrix is actually at different sal
title('SW density (rho, kg/m^3) as function of S and T')




%BELOW THIS POINT FOR SINGLE VALUE CHECKING AGAINST PUBLISHED DATA IN PAPER-------------------
% k=1; j=1;
% temp = 25.004; %CHANGE
% sal = 33.776; %CHANGE
% 
% a0 = .8211458;
% a1 = -.003959680;
% a2 = .00007305182;
% a3 = -.0000008282446;
% a4 = .000000005386657;
% a5 = 0;
% b0 = -.006058307;
% b1 = .00008265457;
% b2 = -.000001077747;
% c0 = .000526528;
% 
% clear rho;
% K = temp(k);% + 273.15;
% A = a0 + a1*(K) + a2*((K)^2) + a3*((K)^3) + a4*((K)^4) + a5*((K)^5)
% B = b0 + b1*(K) + b2*((K)^2)
%         C = c0 
%         rho0 = 999.842594 +.06793952*(temp(k)) - .009095290*(temp(k)^2) + .0001001685*(temp(k)^3) - .000001120083*(temp(k)^4) + .000000006536336*(temp(k)^5) 
%         rho = (A*sal(j) + B*((sal(j))^1.5) + C*((sal(j))^2)) 
% 
