%methods for calculating solubility of seawater as a function of salinity/temperature
%set these four parameters and then run-------------------
tempmin = 8;
tempmax = 40;
salmin = 0.1; 
salmax = 140;
%--------------------------------------------------------


temp=[tempmin:.1:tempmax]';
sal=[salmin:.1:salmax]';

%at saturation using Richards (1965) from 598 hypoxia study, EQUATIONS OLD AND INACCURATE----------------
ml = nan(length(temp),length(sal));
mg = nan(length(temp),length(sal));
for i = 1:length(temp)
    for j = 1:length(sal)
        ml(i,j)=(9.9096-0.2759*temp(i)+0.005396*(temp(i)^2)-0.00004527*(temp(i)^3)-(0.05896-0.00179*temp(i)+0.00002618*(temp(i)^2))*sal(j));
        mg(i,j)=ml(i,j)*(32/(((temp(i)+273.15)/273.15)*22.4));
    end
end

temp2=repmat(temp,length(sal),1); %temp is rows of mg
sal2=reshape((repmat(sal,1,length(temp))'),size(temp,1)*size(sal,1),1); %sal is columns of mg
mg_array=reshape(mg,[size(mg,1)*size(mg,2),1]); %mg 501r x 1501c

%interpolation gride for surf plotting purposes only
[tempq,salq] = meshgrid(tempmin:((tempmax-tempmin)/100):tempmax, salmin:((salmax-salmin)/100):salmax);
mgq = griddata(temp2,sal2,mg_array,tempq,salq); % *q interpolated values evenly spaced

figure
y=surf(tempq,salq,mgq);
box on;
set(y,'LineStyle','none');
xlabel('Temperature (C)');ylabel('Salinity (ppt)');zlabel('Dissolved oxygen at saturation (mg/L)')  %OLD switch x and y labels b/c "x-axis" of mg matrix is actually at different sal
title('Richards 1965')

%TEST AGAINST PLOT3 to confirm
% figure,hold on
% y1=plot3(temp2,sal2,mg_array,'o','MarkerSize',1);
% box on, xlabel('Temperature (C)');ylabel('Salinity (ppt)');zlabel('Dissolved oxygen at saturation (mg/L)')  %OLD switch x and y labels b/c "x-axis" of mg matrix is actually at different sal



%at 30% saturation - NOT UPDATED YET
for i = 1:length(temp)
    for j = 1:length(sal)
        ml(i,j)=(9.9096-0.2759*temp(i)+0.005396*(temp(i)^2)-0.00004527*(temp(i)^3)-(0.05896-0.00179*temp(i)+0.00002618*(temp(i)^2))*sal(j))*.3;
        mg(i,j)=ml(i,j)*(32/(((temp(i)+273.15)/273.15)*22.4));
    end
end

figure
z=surf(temp,sal,mg);
box on;
set(z,'LineStyle','none');
ylabel('Temperature (C)');xlabel('Salinity (ppt)');zlabel('Dissolved oxygen at 30% saturation (mg/L)')  %switch x and y labels b/c "x-axis" of mg matrix is actually at different sal




%Benson and Krause 1984, Limnol. Oceanogr. 29, 620-632-------------------------------------------------------------------
%KELVIN!!!
tempplot=[tempmin:.1:tempmax]';
temp=tempplot+273.15; %convert to K for equations
sal=[salmin:.1:salmax]';

lnC = nan(length(temp),length(sal));
C = nan(length(temp),length(sal));
for i = 1:length(temp)
    for j = 1:length(sal)
        lnC(i,j) = -135.29996 + 1.572288*(100000/temp(i)) - 6.637149*10000000/(temp(i)^2) + 1.243678*10000000000/(temp(i)^3) - 8.621061*100000000000/(temp(i)^4) - (sal(j))*(0.020573 - 12.142/(temp(i)) + 2363.1/(temp(i)^2));
        place = lnC(i,j);        
        C(i,j)=exp(place);
    end
end
C = C/1000*31.9988; %convert umol/kg to mg/L, assuming density of 1kg/L... for now
temp2=repmat(tempplot,length(sal),1); %temp is rows of mg
sal2=reshape((repmat(sal,1,length(tempplot))'),size(tempplot,1)*size(sal,1),1); %sal is columns of mg
C_array=reshape(C,[size(C,1)*size(C,2),1]); %mg 501r x 1501c

%interpolation gride for surf plotting purposes only
[tempq,salq] = meshgrid(tempmin:((tempmax-tempmin)/100):tempmax, salmin:((salmax-salmin)/100):salmax);
Cq = griddata(temp2,sal2,C_array,tempq,salq); % *q interpolated values evenly spaced

figure
y=surf(tempq,salq,Cq);
box on;
set(y,'LineStyle','none');
xlabel('Temperature (C)');ylabel('Salinity (ppt)');zlabel('Dissolved oxygen at saturation (mg/L)')  %OLD switch x and y labels b/c "x-axis" of mg matrix is actually at different sal
title('Benson and Krause 1984')




%BETTER SOLUTION----------------------------------------------
%solubility at 100% air equilibration ("saturation") using Debelius et al. (2009) Hydrobiologia-------
%validated from 0 to 133 o/oo with most deviation <<10C---------------------
a0=-173.9321;
b0=-0.04547;
c0=0.00006644;
a1=256.408;
b1=0.01588;
c1=-0.00001928;
a2=146.3559;
b2=-0.0012;
a3=-22.2471;
a4=0.001186;

tempplot=[tempmin:.1:tempmax]';
temp=tempplot+273.15; %converto K for equations
sal=[salmin:.1:salmax]';

lnC = nan(length(temp),length(sal));
C = nan(length(temp),length(sal));
for i = 1:length(temp)
    for j = 1:length(sal)
        %first calculates ln of solubility (umol / kg-1 at 1atm, S = o/oo,
        %temp = degrees C???
        lnC(i,j)= a0 + a1*(100/(temp(i))) + a2*(log((temp(i))/100)) + a3*((temp(i))/100) + a4*(((temp(i))/100)^2) + (sal(j))*(b0 + b1*((temp(i))/100) + b2*(((temp(i))/100)^2)) + ((sal(j))^2)*(c0 + c1*((temp(i))/100));
        place = lnC(i,j);        
        C(i,j)=exp(place); %exp(lnC(i,j)); move outside loop??
    end
end
C = C/1000*31.9988; %umol/kg to mg/kg   exp(lnC);

temp2=repmat(tempplot,length(sal),1); %temp is rows of mg
sal2=reshape((repmat(sal,1,length(tempplot))'),size(temp,1)*size(sal,1),1); %sal is columns of mg
C_array=reshape(C,[size(C,1)*size(C,2),1]); %mg 501r x 1501c

%interpolation gride for surf plotting purposes only
[tempqD,salqD] = meshgrid(tempmin:((tempmax-tempmin)/100):tempmax, salmin:((salmax-salmin)/100):salmax);
CqD = griddata(temp2,sal2,C_array,tempqD,salqD); % *q interpolated values evenly spaced

figure
y=surf(tempqD,salqD,CqD);
box on;
set(y,'LineStyle','none');
xlabel('Temperature (C)');ylabel('Salinity (ppt)');zlabel('Dissolved oxygen at saturation (mg/L)')  %OLD switch x and y labels b/c "x-axis" of mg matrix is actually at different sal



%OLD BELOW HERE----------------
figure,surf(temp,sal,C), box on
ylabel('Temperature (C)');xlabel('Salinity (ppt)');zlabel('Dissolved oxygen at 30% saturation (mg/L)')  %switch x and y labels b/c "x-axis" of mg matrix is actually at different sal



% function createfigure(xdata1, zdata1)
% %CREATEFIGURE(XDATA1,ZDATA1)
% %  XDATA1:  surface xdata
% %  ZDATA1:  surface zdata
% 
% %  Auto-generated by MATLAB on 11-Nov-2011 08:09:26
% 
% % Create figure
% figure1 = figure;
% 
% % Create axes
% axes1 = axes('Parent',figure1,'XDir','reverse','FontSize',14);
% view(axes1,[-146.5 18]);
% box(axes1,'on');
% grid(axes1,'on');
% hold(axes1,'all');
% 
% % Create surf
% surf(xdata1,xdata1,zdata1,'Parent',axes1,'LineStyle',':');
% 
% % Create xlabel
% xlabel('Temperature (C)','HorizontalAlignment','right','FontSize',16);
% 
% % Create ylabel
% ylabel('Salinity (ppt)','FontSize',16,'HorizontalAlignment','left');
% 
% % Create zlabel
% zlabel('Oxygen saturation (mg/L)','FontSize',16);
