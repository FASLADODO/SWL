p = [
2.300352e-002 0.000000e+000 0.000000e+000 0.000000e+000 1.481397e-001 1.573704e-001 5.441865e-002 1.038168e-001 4.687272e-003 0.000000e+000 7.956671e-002 0.000000e+000 0.000000e+000 9.409520e-002 0.000000e+000 0.000000e+000 2.731613e-002 0.000000e+000 0.000000e+000 0.000000e+000 4.224832e-002 0.000000e+000 1.699337e-001 5.825813e-003 3.243513e-002 0.000000e+000 2.787302e-002 0.000000e+000 2.926962e-002 0.000000e+000
];
Mu = [
-4.552075e+001 1.959963e+000
0.000000e+000 0.000000e+000
0.000000e+000 0.000000e+000
0.000000e+000 0.000000e+000
-2.696496e+001 -5.384102e+000
-1.681657e+000 -2.993957e+001
1.721566e+001 -1.440350e+001
3.343194e+001 -1.599501e+001
2.072594e+001 2.458528e+001
0.000000e+000 0.000000e+000
-1.167987e+001 1.869228e+001
0.000000e+000 0.000000e+000
0.000000e+000 0.000000e+000
-1.479010e+001 3.014543e+001
0.000000e+000 0.000000e+000
0.000000e+000 0.000000e+000
-3.763232e+001 -1.292131e+001
0.000000e+000 0.000000e+000
0.000000e+000 0.000000e+000
0.000000e+000 0.000000e+000
-8.341684e+000 4.317805e+001
0.000000e+000 0.000000e+000
2.688222e+001 1.426311e+001
-4.199713e+001 -1.713361e+001
2.959496e+001 3.401756e+001
0.000000e+000 0.000000e+000
-2.062586e+001 -4.103450e+001
0.000000e+000 0.000000e+000
1.075610e+001 -4.475095e+001
0.000000e+000 0.000000e+000
];
Cov = [
4.255233e+000 4.651663e+000   4.651663e+000 5.926286e+001
0.000000e+000 0.000000e+000   0.000000e+000 0.000000e+000
0.000000e+000 0.000000e+000   0.000000e+000 0.000000e+000
0.000000e+000 0.000000e+000   0.000000e+000 0.000000e+000
5.336596e+001 -7.386587e+001   -7.386587e+001 2.720377e+002
2.872911e+002 -2.057252e+001   -2.057252e+001 3.950910e+001
5.070157e+001 5.529364e+001   5.529364e+001 7.253255e+001
4.972882e+001 1.448981e+001   1.448981e+001 1.058276e+002
1.667843e+001 -9.732864e+001   -9.732864e+001 5.681800e+002
0.000000e+000 0.000000e+000   0.000000e+000 0.000000e+000
1.798937e+002 8.804528e+001   8.804528e+001 4.863817e+001
0.000000e+000 0.000000e+000   0.000000e+000 0.000000e+000
0.000000e+000 0.000000e+000   0.000000e+000 0.000000e+000
2.364171e+002 5.766574e+001   5.766574e+001 3.510872e+001
0.000000e+000 0.000000e+000   0.000000e+000 0.000000e+000
0.000000e+000 0.000000e+000   0.000000e+000 0.000000e+000
1.087290e+001 -2.549866e+001   -2.549866e+001 8.654704e+001
0.000000e+000 0.000000e+000   0.000000e+000 0.000000e+000
0.000000e+000 0.000000e+000   0.000000e+000 0.000000e+000
0.000000e+000 0.000000e+000   0.000000e+000 0.000000e+000
8.839811e+001 8.462634e+000   8.462634e+000 7.408976e+000
0.000000e+000 0.000000e+000   0.000000e+000 0.000000e+000
1.129735e+002 -4.966121e+001   -4.966121e+001 1.043623e+002
4.822681e+000 -9.724766e+000   -9.724766e+000 2.140326e+001
3.649194e+001 -2.096010e+001   -2.096010e+001 1.905848e+001
0.000000e+000 0.000000e+000   0.000000e+000 0.000000e+000
9.177173e+001 -3.738388e+001   -3.738388e+001 1.713966e+001
0.000000e+000 0.000000e+000   0.000000e+000 0.000000e+000
1.024249e+002 2.736931e+001   2.736931e+001 1.089654e+001
0.000000e+000 0.000000e+000   0.000000e+000 0.000000e+000
];

nonzero_elem_indexes = find(p);
p2 = p(nonzero_elem_indexes);
Mu2 = Mu(nonzero_elem_indexes, :);
Cov2 = Cov(nonzero_elem_indexes, :);

Sigma = zeros(2, 2, size(Cov, 1));
for ii = 1:size(Cov, 1)
	Sigma(:,:,ii) = reshape(Cov(1,:), [2,2]);
end;

Sigma2 = zeros(2, 2, size(Cov2, 1));
for ii = 1:size(Cov2, 1)
	Sigma2(:,:,ii) = reshape(Cov2(1,:), [2,2]);
end;

gmm = gmdistribution(Mu2, Sigma2, p2);

figure;
%ezsurf(@(x,y) pdf(gmm, [x y]), [-50 50 -50 50], 2000);
ezcontour(@(x,y) pdf(gmm, [x y]), [-50 50 -50 50], 2000);

kk = 4;
%pd = makedist('Normal', 'mu', Mu2(kk,:), 'sigma', Sigma2(:,:,kk))

figure;
x1 = -50:.1:50; x2 = -50:.1:50;
[X1, X2] = meshgrid(x1, x2);
F = mvnpdf([X1(:) X2(:)], Mu2(kk,:), Sigma2(:,:,kk));
%F = mvnpdf([X1(:) X2(:)], Mu2, Sigma2);
F = reshape(F, length(x2), length(x1));
%surf(x1, x2, F);
contour(x1, x2, F);
caxis([min(F(:))-.5*range(F(:)), max(F(:))]);
axis([-50 50 -50 50 0 .4])
xlabel('x1'); ylabel('x2'); zlabel('Probability Density');
